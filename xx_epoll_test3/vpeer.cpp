#include "vpeer.h"
#include "gpeer.h"

VPeerCB::VPeerCB(VPeer *vpeer, int serial, std::function<void(uint8_t const *buf, size_t len)> &&cbFunc, double timeoutSeconds)
        : EP::Timer(vpeer->server, -1), vpeer(vpeer), serial(serial), func(std::move(cbFunc)) {
    SetTimeoutSeconds(timeoutSeconds);
}

void VPeerCB::Timeout() {
    // 模拟超时
    func(nullptr, 0);
    // 从容器移除
    vpeer->callbacks.erase(serial);
}

VPeer::VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId)
        : server(server), gatewayPeer(gatewayPeer), clientId(clientId) {
}

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

int VPeer::SendRequest(uint8_t const *const &buf, size_t const &len,
                       std::function<void(uint8_t const *const &buf, size_t const &len)> &&cbfunc,
                       double const &timeoutSeconds) {
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

void VPeer::Kick(int const &reason, std::string_view const &desc) {
    if (!Alive()) return;
    LOG_INFO("VPeer Kick, reason = ", reason, ", desc = ", desc);

    // 触发所有已存在回调（ 模拟超时回调 ）
    for (auto &&iter : callbacks) {
        iter.second->func(nullptr, 0);
    }
    callbacks.clear();

    // cmd to gateway: close, sync gateway clientIds
    gatewayPeer->SendClose(clientId);
    gatewayPeer->clientIds.erase(clientId);

    gatewayPeer = nullptr;
    clientId = --server->autoDecClientId;

    server->vps.UpdateAt(serverVpsIndex, clientId);
}

void VPeer::SwapWith(int const& idx) {
    auto& a = server->vps.ValueAt(serverVpsIndex);
    assert(a);
    assert(a.pointer == this);
    auto& b = server->vps.ValueAt(idx);
    assert(b);
    std::swap(a->serverVpsIndex, b->serverVpsIndex);
    std::swap(a->gatewayPeer, b->gatewayPeer);
    std::swap(a->clientId, b->clientId);
    std::swap(a.pointer, b.pointer);
}

void VPeer::Dispose() {
    Kick(__LINE__, "Dispose");
    // destruct
    assert(serverVpsIndex != -1);
    assert(server->vps.ValueAt(serverVpsIndex).pointer == this);
    server->vps.RemoveAt(serverVpsIndex);
    // no more code here
}

void VPeer::ReceivePush(uint8_t const *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    LOG_INFO("VPeer ClientId = ", clientId, " ReceivePush ", dr);
    // todo
}

void VPeer::ReceiveRequest(int const &serial, uint8_t const *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    LOG_INFO("VPeer ClientId = ", clientId, " ReceiveRequest ", dr);
    // todo
}

void VPeer::Update(double const& dt) {
    // todo
}
