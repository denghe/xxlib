#include "vpeer.h"
#include "gpeer.h"
#include "pkg_lobby.h"
#include "pkg_db.h"
//#include "pkg_game.h"
#include "dbpeer.h"
#include "lpeer.h"

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
}

void VPeer::Update(double const &dt) {
    if (IsGuest()) return;
    // todo: frame logic here
}
