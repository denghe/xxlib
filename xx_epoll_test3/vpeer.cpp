#include "vpeer.h"
#include "gpeer.h"
#include "pkg_lobby.h"
#include "pkg_db.h"
#include "dbpeer.h"
#include "game.h"

#define S ((Server*)ec)

// 发回应 package
int VPeer::SendResponse(int32_t const &serial, xx::ObjBase_s const &ob) {
    if (!Alive()) return __LINE__;

    // 准备发包填充容器
    xx::Data d(16384);
    // 跳过包头
    d.len = sizeof(uint32_t);
    // 写 要发给谁
    d.WriteFixed(clientId);
    // 写序号
    d.WriteVarInteger(serial);
    // 写数据
    S->om.WriteTo(d, ob);
    // 填包头
    *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
    // 发包并返回
    return gatewayPeer->Send(std::move(d));
}

void VPeer::Receive(uint8_t const *const &buf, size_t const &len) {
    // drop data when offline
    if (!Alive()) return;

    // 试读出序号. 出错直接断开退出
    int serial = 0;
    xx::Data_r dr(buf, len);
    if (int r = dr.Read(serial)) {
        Kick(__LINE__, xx::ToString("dr.Read(serial) r = ", r));
        return;
    }

    // unpack
    xx::ObjBase_s ob;
    if (int r = S->om.ReadFrom(dr, ob)) {
        S->om.KillRecursive(ob);
        Kick(__LINE__, xx::ToString("S->om.ReadFrom(dr, ob) r = ", r));
    }

    // ensure
    if (!ob || ob.typeId() == 0) {
        Kick(__LINE__, xx::ToString("!ob || ob.typeId() == 0"));
    }

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

bool VPeer::Alive() const {
    return gatewayPeer != nullptr;
}

bool VPeer::Close(int const &reason, std::string_view const &desc) {
    if (serverVpsIndex == -1) return false;
    Kick(__LINE__, "Dispose");
    assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
    S->vps.RemoveAt(serverVpsIndex);
    serverVpsIndex = -1;
    DelayUnhold();
    return true;
}

void VPeer::Kick(int const &reason, std::string_view const &desc, bool const &fromGPeerClose) {
    if (!Alive()) return;
    LOG_INFO("reason = ", reason, ", desc = ", desc);

    // 触发所有已存在回调（ 模拟超时回调 ）
    ClearCallbacks();

    if (!fromGPeerClose) {
        // cmd to gateway: close, sync gateway clientIds
        gatewayPeer->SendClose(clientId);
        gatewayPeer->clientIds.erase(clientId);
    }
    gatewayPeer = nullptr;
    clientId = --S->autoDecId;

    assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
    S->vps.UpdateAt<0>(serverVpsIndex, clientId);
}

void VPeer::SwapWith(int const &idx) {
    auto &a = S->vps.ValueAt(serverVpsIndex);
    assert(a);
    assert(a.pointer == this);

    auto &b = S->vps.ValueAt(idx);
    assert(b);
    assert(a.pointer != b.pointer);

    std::swap(a->serverVpsIndex, b->serverVpsIndex);
    std::swap(a->gatewayPeer, b->gatewayPeer);
    std::swap(a->clientId, b->clientId);
    std::swap(a->ip, b->ip);
    std::swap(a->autoIncSerial, b->autoIncSerial);

    // do not swap logic data

    std::swap(a.pointer, b.pointer);
}


xx::Weak<VPeer> VPeer::Weak() {
    return xx::SharedFromThis(this).ToWeak();
}

/****************************************************************************************/
// accountId? logic code here

VPeer::VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId, std::string &&ip)
        : EP::Timer(server), gatewayPeer(gatewayPeer), clientId(clientId), ip(std::move(ip)) {
    accountId = --server->autoDecId;
    auto r = server->vps.Add(this, ((uint64_t) gatewayPeer->gatewayId << 32) | clientId, accountId);
    assert(r.success);
    serverVpsIndex = r.index;
    assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
    Hold();
    gatewayPeer->SendOpen(clientId);
}

bool VPeer::IsGuest() const {
    return accountId < 0;
}

void VPeer::Timeout() {
    // todo: logic here ?
    // Kick(__LINE__, "Timeout");
}

void VPeer::ReceivePush(xx::ObjBase_s &&ob) {
    if (IsGuest()) {
        LOG_ERR("clientId = ", clientId, " (Guest) ob = ", S->om.ToString(ob));
    } else {
        LOG_INFO("clientId = ", clientId, ", accountId = ", accountId, " ob = ", S->om.ToString(ob));
        // todo: logic here
    }
}

void VPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_INFO("clientId = ", clientId, " accountId = ", accountId, " ob = ", S->om.ToString(ob));
    if (IsGuest()) {
        // guest logic here: auth login

        // switch handle
        switch (ob.typeId()) {
            case xx::TypeId_v<Client_Lobby::Auth>: {
                auto &&o = S->om.As<Client_Lobby::Auth>(ob);

                // ensure dbpeer
                if (!S->dbPeer || !S->dbPeer->Alive()) {
                    auto &&m = InstanceOf<Lobby_Client::Auth::Error>();
                    m->errorCode = -1;
                    m->errorMessage = "can't connect to db server";
                    SendResponse(serial, m);
                    return;
                }

                // check & set busy flag
                if (TypeCount<Client_Lobby::Auth>()) {
                    Close(__LINE__, "VPeer::ReceiveRequest duplicated Client_Lobby::Auth");
                    return;
                } else {
                    TypeCountInc<Client_Lobby::Auth>();
                }

                {
                    auto &&m = xx::Make<Lobby_Database::GetAccountInfoByUsernamePassword>();
                    m->username = std::move(o->username);
                    m->password = std::move(o->password);
                    S->dbPeer->SendRequest(m, [this, serial](xx::ObjBase_s &&ob) {
                        // check & clear busy flag
                        assert(TypeCount<Client_Lobby::Auth>() == 1);
                        TypeCountDec<Client_Lobby::Auth>();

                        // timeout?
                        if (!ob) {
                            // send error
                            auto &&m = InstanceOf<Lobby_Client::Auth::Error>();
                            m->errorCode = -2;
                            m->errorMessage = "db server response timeout";
                            SendResponse(serial, m);
                            return;
                        }

                        // handle result
                        switch (ob.typeId()) {
                            case xx::TypeId_v<Database_Lobby::GetAccountInfoByUsernamePassword::Error>: {
                                auto &&o = S->om.As<Database_Lobby::GetAccountInfoByUsernamePassword::Error>(ob);

                                // send error
                                auto &&m = InstanceOf<Lobby_Client::Auth::Error>();
                                m->errorCode = o->errorCode;
                                m->errorMessage = std::move(o->errorMessage);
                                SendResponse(serial, m);
                                return;
                            }
                            case xx::TypeId_v<Database_Lobby::GetAccountInfoByUsernamePassword::Result>: {
                                auto &&o = S->om.As<Database_Lobby::GetAccountInfoByUsernamePassword::Result>(ob);

                                // can't find user: send error
                                if (!o->accountInfo.has_value()) {
                                    auto &&m = InstanceOf<Lobby_Client::Auth::Error>();
                                    m->errorCode = -1;
                                    m->errorMessage = "bad username or password";
                                    SendResponse(serial, m);
                                    return;
                                }

                                int r = SetAccount(*o->accountInfo);
                                if (r < 0) {

                                    // send error
                                    auto &&m = InstanceOf<Lobby_Client::Auth::Error>();
                                    m->errorCode = -2;
                                    m->errorMessage = xx::ToString("SetAccountId error. accountId = ", o->accountInfo->accountId, " r = ", r);
                                    SendResponse(serial, m);
                                } else {

                                    // success
                                    auto fill = [&](Lobby_Client::Auth::Online* const& m) {
                                        m->accountId = o->accountInfo->accountId;
                                        m->nickname = o->accountInfo->nickname;
                                        m->coin = o->accountInfo->coin;
                                        m->games.clear();
                                        for (auto& kv : S->games) {
                                            auto& g = m->games.emplace_back();
                                            g.gameId = kv.first;
                                        }
                                    };
                                    if (r == 0) {

                                        // send auth result: online
                                        auto &&m = InstanceOf<Lobby_Client::Auth::Online>();
                                        fill(m);
                                        SendResponse(serial, m);
                                    } else {

                                        // send auth result: restore
                                        auto &&m = InstanceOf<Lobby_Client::Auth::Restore>();
                                        fill(m);
                                        m->gameId = game->gameId;
                                        //m->serviceId = vp->game->peer->serviceId; // todo: fill
                                        SendResponse(serial, m);
                                    }
                                }
                                return;
                            }
                        }
                    }, 15);
                }

            } // case

            default:
                break;
        }
    } else {
        // todo: game logic here
    }
}

int VPeer::SetAccount(Database::AccountInfo const &ai) {
    // ensure current is guest mode
    if (accountId != -1) return -__LINE__;
    // validate args
    if (accountId == ai.accountId) return -__LINE__;
    // check exists
    int idx = S->vps.Find<1>(ai.accountId);
    if (idx == -1) {
        // not found: update key, online
        assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
        auto r = S->vps.UpdateAt<1>(serverVpsIndex, ai.accountId);
        assert(r);
        accountId = ai.accountId;
        nickname = ai.nickname;
        coin = ai.coin;
        return 0;
    } else {
        // exists: kick & swap
        auto &tar = S->vps.ValueAt(idx);
        assert(tar->accountId == ai.accountId);
        assert(tar->nickname == ai.nickname);
        assert(tar->coin == ai.coin);
        tar->Kick(__LINE__, xx::ToString("replace by gatewayId = ", gatewayPeer->gatewayId, " clientId = ", clientId, " ip = ", ip));
        SwapWith(idx);
        return 1;
    }
}

void VPeer::Update(double const &dt) {
    if (IsGuest()) return;
    // todo: frame logic here
}
