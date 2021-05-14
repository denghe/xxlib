#include "peer.h"
#include "server.h"
#include "db.h"
#include "pkg_db.h"

#define S ((Server*)ec)

bool Peer::Close(int const &reason, std::string_view const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;

    ClearCallbacks();

    // 延迟减持
    DelayUnhold();
    return true;
}

int Peer::SendResponse(int32_t const &serial, xx::ObjBase_s const &ob) {
    if (!Alive()) return __LINE__;
    // 准备发包填充容器
    xx::Data d(65536);
    // 跳过包头
    d.len = sizeof(uint32_t);
    // 写序号
    d.WriteVarInteger(serial);
    // 写数据
    S->om.WriteTo(d, ob);
    // 填包头
    *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
    // 发包并返回
    Send(std::move(d));
    return 0;
}

void Peer::Receive() {
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
            int32_t serial = 0;
            if (int r = dr.Read(serial)) {
                LOG_ERR("dr.Read(serial) r = ", r);
            }
            else {
                // unpack
                xx::ObjBase_s ob;
                if ((r = S->om.ReadFrom(dr, ob))) {
                    LOG_ERR("S->om.ReadFrom(dr, ob) r = ", r);
                    S->om.KillRecursive(ob);
                }
                if (!ob || ob.typeId() == 0) {
                    LOG_ERR("ob is nullptr or typeId == 0");
                }
                else {
                    // 根据序列号的情况分性质转发
                    if (serial == 0) {
                        ReceivePush(std::move(ob));
                    } else if (serial > 0) {
                        ReceiveResponse(serial, std::move(ob));
                    } else {
                        // -serial: 将 serial 转为正数
                        ReceiveRequest(-serial, std::move(ob));
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

/****************************************************************************************/

void Peer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package:", S->om.ToString(ob));
}

void Peer::ReceiveRequest(int32_t const &serial, xx::ObjBase_s &&ob) {
    switch(ob.typeId()) {
        case xx::TypeId_v<Lobby_Database::GetAccountInfoByUsernamePassword>: {
            auto &&o = S->om.As<Lobby_Database::GetAccountInfoByUsernamePassword>(ob);
            // put job to thread pool
            // thread safe: o => shared_ptr( use ), copy serial( copy through ), vpper => weak( move through )
            S->db->AddJob([o = S->ToPtr(std::move(o)), serial, w = xx::SharedFromThis(this).ToWeak()](DB::Env &env) mutable {

                // SQL query
                auto rtv = env.TryGetAccountInfoByUsernamePassword(o->username, o->password);

                // dispatch handle SQL query result
                // thread safe: copy serial( use ), move rtv( use ), move w( use )
                env.server->Dispatch([serial, rtv = std::move(rtv), w = std::move(w)]() mutable {

                    // ensure p is exists & alive
                    if (auto p = w.Lock(); p->Alive()) {

                        // check result
                        if (!rtv) {

                            // send sql execute error
                            auto &&m = p->InstanceOf<Database_Lobby::GetAccountInfoByUsernamePassword::Error>();
                            m->errorCode = rtv.errorCode;
                            m->errorMessage = rtv.errorMessage;
                            p->SendResponse(serial, m);

                        } else {

                            // send result
                            auto &&m = p->InstanceOf<Database_Lobby::GetAccountInfoByUsernamePassword::Result>();
                            if (rtv.value.accountId >= 0) {
                                m->accountInfo.emplace();
                                m->accountInfo->accountId = rtv.value.accountId;
                                m->accountInfo->nickname = rtv.value.nickname;
                                m->accountInfo->coin = rtv.value.coin;
                            }
                            p->SendResponse(serial, m);

                        }
                    }
                });
            });
            return;
        }
        default:
            LOG_ERR("unhandled package: ", S->om.ToString(ob));
    }
}
