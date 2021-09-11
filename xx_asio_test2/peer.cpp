#include "peer.h"
#include "server.h"
#include "ss.h"

void Peer::Receive() {
    // 取出指针备用
    auto buf = recv.buf;
    auto end = recv.buf + recv.len;
    uint32_t dataLen = 0;

    // 确保包头长度充足
    while (buf + sizeof(dataLen) <= end) {
        // 取长度
        dataLen = *(uint32_t *) buf;

        // 长度异常则断线退出
        if (dataLen < 1 || dataLen > 1024 * 256) {
            Close(-__LINE__, " Peer Receive if (dataLen < 1 || dataLen > 1024 * 256)");
            return;
        }

        // 数据未接收完 就 跳出
        if (buf + sizeof(dataLen) + dataLen > end) break;

        // 跳到数据区开始调用处理回调
        buf += sizeof(dataLen);
        {
            // unpack & handle
            xx::Data_r dr(buf, dataLen);
            xx::ObjBase_s o;
            int r = ((Server *)ec)->om.ReadFrom(dr, o);
            if (r || !o) {
                Close(-__LINE__, xx::ToString(" Peer Receive om.ReadFrom r = ", r));
                return;
            }
            r = ((Server *)ec)->HandlePackage(*this, o);
            if (r) {
                ((Server *)ec)->om.RecursiveReset(o);
                Close(-__LINE__, xx::ToString(" Peer Receive HandlePackage error. r = ", r));
                return;
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

bool Peer::Close(int const& reason, std::string_view const& desc) {
    // 防重入( 同时关闭 fd )
    if (!this->BaseType::Close(reason, desc)) return false;
    LOG_INFO("ip = ", addr, ", clientId = ", clientId ,", reason = ", reason, ", desc = ", desc);
    // 延迟减持
    DelayUnhold();
    // call server logic
    ((Server *)ec)->Kick(clientId);
    return true;
}

void Peer::Send(xx::ObjBase_s const& o) {
    auto& d = ((Server *)ec)->tmp;
    ((Server *)ec)->WriteTo(d, o);
    Send(d);
}

void Peer::Send(xx::Data const& d) {
    BaseType::Send(d.buf, d.len);
    Flush();
}
