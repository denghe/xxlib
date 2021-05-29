#include "vpeer.h"
#include "gpeer.h"
#include "pkg_lobby_client.h"
#include "pkg_db_service.h"
//#include "pkg_game.h"
#include "dbpeer.h"
#include "lpeer.h"

#define S ((Server*)ec)

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

void VPeer::Timeout() {
    // todo: logic here ?
    // Kick(__LINE__, "Timeout");
}

void VPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_INFO("clientId = ", clientId, ", accountId = ", info.accountId, " ob = ", S->om.ToString(ob));
    // todo: logic here
}

void VPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_INFO("clientId = ", clientId, " accountId = ", info.accountId, " ob = ", S->om.ToString(ob));
}

void VPeer::Update(double const &dt) {
    coros();
    // todo: frame logic here
}
