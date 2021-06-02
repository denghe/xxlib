#include "vpeer.h"
#include "gpeer.h"
#include "pkg_lobby_client.h"
#include "pkg_db_service.h"
#include "dbpeer.h"
#include "game.h"
#include "speer.h"

#define S ((Server*)ec)

void VPeer::Receive(uint8_t const *const &buf, size_t const &len) {
    // drop data when offline
    if (!Alive()) return;

    // 试读出序号. 出错直接断开退出
    int serial = 0;
    xx::Data_r dr(buf, len);
    LOG_INFO("clientId = ", clientId, ", buf = ", dr);
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

/****************************************************************************************/
// accountId? logic code here

VPeer::VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId, std::string &&ip)
        : EP::Timer(server), gatewayPeer(gatewayPeer), clientId(clientId), ip(std::move(ip)), coros(server->frameRate, server->frameRate * 60 * 5) {
    info.accountId = --server->autoDecId;
    gatewayPeer->clientIds.insert(clientId);
    auto r = server->vps.Add(this, ((uint64_t) gatewayPeer->gatewayId << 32) | clientId, info.accountId);
    assert(r.success);
    serverVpsIndex = r.index;
    assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
    Hold();
    gatewayPeer->SendOpen(clientId);
}

bool VPeer::IsGuest() const {
    return info.accountId < 0;
}

void VPeer::Timeout() {
    // todo: logic here ?
    // Kick(__LINE__, "Timeout");
}

void VPeer::ReceivePush(xx::ObjBase_s &&ob) {
    if (IsGuest()) {
        LOG_ERR("clientId = ", clientId, " (Guest) ob = ", S->om.ToString(ob));
    } else {
        LOG_INFO("clientId = ", clientId, ", accountId = ", info.accountId, " ob = ", S->om.ToString(ob));
        // todo: logic here
    }
}

void VPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    if (IsGuest()) {
        // guest logic here: auth login

        // switch handle
        switch (ob.typeId()) {
            case xx::TypeId_v<Client_Lobby::Auth>: {
                // check flag
                if (flag_Auth) {
                    Close(__LINE__, "VPeer::ReceiveRequest already authing...");
                    return;
                }

                // ensure dbpeer
                if (!S->dbPeer || !S->dbPeer->Alive()) {
                    SendResponse<Generic::Error>(serial, -1, "can't connect to db server");
                    return;
                }

                // set flag
                flag_Auth = true;
                coroSerial = serial;
                coroOb = std::move(ob);
                coros.Add(HandleRequest_Auth());//serial, &ob));//std::move(ob)));
                return;
            } // case

            default:
                LOG_ERR("unhandled package: ", S->om.ToString(ob));
        }
    } else {
        // todo: game logic here
    }
}

xx::Coro VPeer::HandleRequest_Auth() {
    auto serial = coroSerial;
    auto ob = std::move(coroOb);
    S->om.CoutN(serial, ob);
    // todo: bug fix here: ob == nullptr
    auto &&a = S->om.As<Client_Lobby::Auth>(ob);
    assert(a);

    xx::ObjBase_s rtv;
    co_yield xx::Cond(15).Event( DbCoroSendRequest<Service_Database::GetAccountInfoByUsernamePassword>(rtv, a->username, a->password) );

    // clear flag
    flag_Auth = false;

    // timeout?
    if (!rtv) {
        // send error
        SendResponse<Generic::Error>(serial, -1, "db server response timeout");
        co_return;
    }

    // handle result
    switch (rtv.typeId()) {
        case xx::TypeId_v<Generic::Error>: {
            // send error
            SendResponse(serial, rtv);
            co_return;
        }
        case xx::TypeId_v<Database_Service::GetAccountInfoByUsernamePasswordResult>: {
            auto &&o = S->om.As<Database_Service::GetAccountInfoByUsernamePasswordResult>(rtv);

            // can't find user: send error
            if (!o->accountInfo.has_value()) {
                SendResponse<Generic::Error>(serial, -2, "bad username or password");
                co_return;
            }

            int r = Online(*o->accountInfo);
            if (r < 0) {

                // error
                SendResponse<Generic::Error>(serial, -3, xx::ToString("SetAccountId error. accountId = ", o->accountInfo->accountId, " r = ", r));
            } else {
                {
                    // success
                    auto &&m = S->FromCache<Lobby_Client::PlayerContext>();
                    m->self.accountId = o->accountInfo->accountId;
                    m->self.nickname = o->accountInfo->nickname;
                    m->self.coin = o->accountInfo->coin;
                    m->gameId = gameId;
                    m->serviceId = serviceId;
                    SendResponse(serial, m);
                }

                // push game list cache
                SendPush<xx::Span>(S->data_Lobby_Client_GameOpen);
            }
            co_return;
        }
    }
}


int VPeer::Online(Database::AccountInfo const &ai) {
    // ensure current is guest mode
    if (info.accountId != -1) return -__LINE__;
    // validate args
    if (info.accountId == ai.accountId) return -__LINE__;
    // check exists
    int idx = S->vps.Find<1>(ai.accountId);
    if (idx == -1) {
        // not found: update key, online
        assert(S->vps.ValueAt(serverVpsIndex).pointer == this);
        auto r = S->vps.UpdateAt<1>(serverVpsIndex, ai.accountId);
        assert(r);
        info = ai;
        return 0;
    } else {
        // exists: kick & swap
        auto &tar = S->vps.ValueAt(idx);
        assert(tar->info == ai);
        tar->Kick(__LINE__, xx::ToString("replace by gatewayId = ", gatewayPeer->gatewayId, " clientId = ", clientId, " ip = ", ip));
        SwapWith(idx);
        return 1;
    }
}

void VPeer::Update(double const &dt) {
    coros();
    if (IsGuest()) return;
    // todo: frame logic here
}
