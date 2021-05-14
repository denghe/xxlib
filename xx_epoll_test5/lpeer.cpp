#include "server.h"
#include "lpeer.h"
#include "pkg_lobby.h"

#define S ((Server*)ec)

bool LPeer::Close(int const &reason, std::string_view const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;

    ClearCallbacks();

    // 延迟减持
    DelayUnhold();
    return true;
}

void LPeer::Receive() {
    // 取出指针备用
    auto buf = recv.buf;
    auto end = recv.buf + recv.len;
    uint32_t dataLen = 0;
    uint32_t addr = 0;

    // 确保包头长度充足
    while (buf + sizeof(dataLen) <= end) {
        // 取长度
        dataLen = *(uint32_t *) buf;

        // 长度异常则断线退出( 不含地址? 超长? 256k 不够可以改长 )
        if (dataLen < sizeof(addr) || dataLen > 1024 * 256) {
            Close(__LINE__, "Peer Receive if (dataLen < sizeof(addr) || dataLen > 1024 * 256)");
            return;
        }

        // 数据未接收完 就 跳出
        if (buf + sizeof(dataLen) + dataLen > end) break;

        // 跳到数据区开始调用处理回调
        buf += sizeof(dataLen);
        {
            xx::Data_r dr(buf, dataLen);
            // 试读出序号
            int serial = 0;
            if (int r = dr.Read(serial)) {
                LOG_ERR("dr.Read(serial) r = ", r);
            }
            else {
                // unpack
                xx::ObjBase_s o;
                if ((r = S->om.ReadFrom(dr, o))) {
                    LOG_ERR("S->om.ReadFrom(dr, o) r = ", r);
                    S->om.KillRecursive(o);
                }
                if (!o || o.typeId() == 0) {
                    LOG_ERR("o is nullptr or typeId == 0");
                }
                else {
                    // 根据序列号的情况分性质转发
                    if (serial == 0) {
                        ReceivePush(std::move(o));
                    } else if (serial > 0) {
                        ReceiveResponse(serial, std::move(o));
                    } else {
                        // -serial: 将 serial 转为正数
                        ReceiveRequest(-serial, std::move(o));
                    }
                }
            }
            // 如果当前类实例 fd 已 close 则退出
            if (!Alive()) return;
        }
        // 跳到下一个包的开头
        buf += dataLen;
    }

    // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
    recv.RemoveFront(buf - recv.buf);
}

int LPeer::SendResponse(int32_t const &serial, xx::ObjBase_s const &o) {
    if (!Alive()) return __LINE__;
    // 准备发包填充容器
    xx::Data d(65536);
    // 跳过包头
    d.len = sizeof(uint32_t);
    // 写序号
    d.WriteVarInteger(serial);
    // 写数据
    S->om.WriteTo(d, o);
    // 填包头
    *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
    // 发包并返回
    Send(std::move(d));
    return 0;
}

/****************************************************************************************/

void LPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}

void LPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
