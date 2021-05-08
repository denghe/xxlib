#pragma once

#include "server.h"

struct GPeer;
struct VPeer;

// 带超时的回调
struct VPeerCB : EP::Timer {
    using Func = std::function<void(uint8_t const *buf, size_t len)>;

    // 所在 vpeer( 移除的时候要用到 )
    VPeer *vpeer;

    // 序号( 移除的时候要用到 )
    int serial = 0;

    // 回调函数本体
    Func func;

    // 继承构造函数
    VPeerCB(VPeer *const &vpeer, int const &serial, Func &&cbFunc, double const &timeoutSeconds);

    // 执行 func(0,0) 后 从容器移除, 并延迟 Unhold
    void Timeout() override;
};

// 虚拟 peer
struct VPeer : EP::Timer {
    explicit VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId);

    // index at server->vps( fill after create )
    int serverVpsIndex = -1;

    // 指向 gateway peer
    GPeer *gatewayPeer;

    // 存放位于 gateway 的 client id
    uint32_t clientId;

    // logic data
    int32_t accountId = -1;

    // 循环自增用于生成 serial
    int autoIncSerial = 0;

    // 所有 带超时的回调. key: serial
    std::unordered_map<int, xx::Shared<VPeerCB>> callbacks;

    /****************************************************************************************/

    // 发推送
    int SendPush(uint8_t const *const &buf, size_t const &len) const;

    // 发回应
    int SendResponse(int const &serial, uint8_t const *const &buf, size_t const &len) const;

    // 发请求（收到相应回应时会触发 cb 执行。超时或断开也会触发，buf == nullptr）
    int SendRequest(uint8_t const *const &buf, size_t const &len, typename VPeerCB::Func &&cbfunc, double const &timeoutSeconds);

    /****************************************************************************************/

    // 收到数据( 进一步解析 serial 并转发到下面几个函数 )
    void Receive(uint8_t const *const &buf, size_t const &len);

    // 收到回应( 自动调用 发送请求时设置的回调函数 )
    void ReceiveResponse(int const &serial, uint8_t const *const &buf, size_t const &len);

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(uint8_t const *const &buf, size_t const &len);

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int const &serial, uint8_t const *const &buf, size_t const &len);

    /****************************************************************************************/

    // gatewayPeer != nullptr
    bool Alive() const;

    // kick, remove from server.vps, DelayUnhold
    bool Close(int const &reason, std::string_view const &desc) override;

    // kick, cleanup, update key at server.vps, remove from gpeer.clientIds
    void Kick(int const &reason, std::string_view const &desc, bool const& fromGPeerClose = false);

    // swap server->vps.ValueAt( serverVpsIndex & idx )'s network ctx
    void SwapWith(int const &idx);

    // logic update here
    void Update(double const &dt);

    // accountId < 0
    bool IsGuest() const;

    void Timeout() override;
};
