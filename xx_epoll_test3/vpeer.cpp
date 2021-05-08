#include "vpeer.h"
#include "gpeer.h"
#include "pkg.h"
#include "db.h"

#define S ((Server*)ec)

/****************************************************************************************/

VPeerCB::VPeerCB(VPeer *const &vpeer, int const &serial, Func &&cbFunc, double const &timeoutSeconds)
        : EP::Timer(vpeer->ec, -1), vpeer(vpeer), serial(serial), func(std::move(cbFunc)) {
    SetTimeoutSeconds(timeoutSeconds);
}

void VPeerCB::Timeout() {
    // 模拟超时
    func(nullptr, 0);
    // 从容器移除
    vpeer->callbacks.erase(serial);
}

/****************************************************************************************/

int VPeer::SendPush(uint8_t const *const &buf, size_t const &len) const {
    // 推送性质的包, serial == 0
    return SendResponse(0, buf, len);
}

int VPeer::SendResponse(int32_t const &serial, uint8_t const *const &buf, size_t const &len) const {
    if (!Alive()) return __LINE__;

    // 准备发包填充容器
    xx::Data d(len + 13);
    // 跳过包头
    d.len = sizeof(uint32_t);
    // 写 要发给谁
    d.WriteFixed(clientId);
    // 写序号
    d.WriteVarInteger(serial);
    // 写数据
    d.WriteBuf(buf, len);
    // 填包头
    *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
    // 发包并返回
    return gatewayPeer->Send(std::move(d));
}

int VPeer::SendRequest(uint8_t const *const &buf, size_t const &len, typename VPeerCB::Func &&cbfunc, double const &timeoutSeconds) {
    // 产生一个序号. 在正整数范围循环自增( 可能很多天之后会重复 )
    autoIncSerial = (autoIncSerial + 1) & 0x7FFFFFFF;
    // 创建一个 带超时的回调
    auto &&cb = xx::Make<VPeerCB>(xx::SharedFromThis(this), autoIncSerial, std::move(cbfunc), timeoutSeconds);
    cb->Hold();
    // 以序列号建立cb的映射
    callbacks[autoIncSerial] = cb;
    // 发包并返回( 请求性质的包, 序号为负数 )
    return SendResponse(-autoIncSerial, buf, len);
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

    // 根据序列号的情况分性质转发
    if (serial == 0) {
        ReceivePush(buf + dr.offset, len - dr.offset);
    } else if (serial > 0) {
        ReceiveResponse(serial, buf + dr.offset, len - dr.offset);
    } else {
        // -serial: 将 serial 转为正数
        ReceiveRequest(-serial, buf + dr.offset, len - dr.offset);
    }
}

void VPeer::ReceiveResponse(int const &serial_, uint8_t const *const &buf, size_t const &len) {
    // 根据序号定位到 cb. 找不到可能是超时或发错?
    auto &&iter = callbacks.find(serial_);
    if (iter == callbacks.end()) return;
    iter->second->func(buf, len);
    callbacks.erase(iter);
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
    for (auto &&iter : callbacks) {
        iter.second->func(nullptr, 0);
    }
    callbacks.clear();

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
    std::swap(a->autoIncSerial, b->autoIncSerial);

    std::swap(a.pointer, b.pointer);
}


xx::Weak<VPeer> VPeer::Weak() {
    return xx::SharedFromThis(this).ToWeak();
}

/****************************************************************************************/
// accountId? logic code here

VPeer::VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId)
        : EP::Timer(server), gatewayPeer(gatewayPeer), clientId(clientId) {
    accountId = --server->autoDecId;
    auto r = server->vps.Add(xx::SharedFromThis(this), ((uint64_t) gatewayPeer->gatewayId << 32) | clientId, accountId);
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

void VPeer::ReceivePush(uint8_t const *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    if (IsGuest()) {
        LOG_ERR("clientId = ", clientId, " (Guest) ReceivePush ", dr);
    } else {
        LOG_INFO("clientId = ", clientId, ", accountId = ", accountId, " ReceivePush ", dr);
        // todo: logic here
    }
}

void VPeer::ReceiveRequest(int const &serial, uint8_t const *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    LOG_INFO("clientId = ", clientId, " accountId = ", accountId, " buf = ", dr);
    if (IsGuest()) {
        // logic here
        xx::ObjBase_s pkg;
        if (int r = S->om.ReadFrom(dr, pkg)) {
            Close(__LINE__, xx::ToString("VPeer ReceiveRequest clientId = ", clientId, " accountId = ", accountId, " if (int r = S->om.ReadFrom(dr, pkg)) r = ", r, " buf = ", dr));
            return;
        }
        switch (pkg.typeId()) {
            case xx::TypeId_v<Client_Lobby::Auth>: {
                auto &&o = S->om.As<Client_Lobby::Auth>(pkg);
                // todo: set busy flag?
                // o -> shared_ptr thread safe
                S->db->AddJob([o = S->ToPtr(std::move(o)), serial, w = Weak()](DB::Env &env) mutable {
                    int aid = env.GetAccountIdByUsernamePassword(o->username, o->password);
                    env.server->Dispatch([serial, aid, w = std::move(w)] {
                        if (auto vp = w.Lock()) {
                            // todo: clear busy flag?
                            auto msg = xx::Make<Lobby_Client::AuthResult>();
                            msg->accountId = aid;
                            auto s = ((Server*)vp->ec);
                            s->d.Clear();
                            s->om.WriteTo(s->d, msg);
                            vp->SendResponse(serial, s->d.buf, s->d.len);
                            if (aid != -1) {
                                // vp->accountId = aid;
                                // todo: update key?
                            }
                            // todo: xx::ObjBase_s version SendXxxxxx
                        }
                    });
                });

                break;
            }
            default: {
                break;
            }
        }
    } else {
        // todo: game logic here
    }
}

void VPeer::Update(double const &dt) {
    if (IsGuest()) return;
    // todo: frame logic here
}
