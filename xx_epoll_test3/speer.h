#pragma once
#include "peer.h"

struct Game;

struct SPeer : Peer {
    using Peer::Peer;

    uint32_t serviceId = 0xFFFFFFFFu;
    Game* game = nullptr;

    // cleanup callbacks, DelayUnhold, remove from server.game container
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&ob) override;

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) override;
};
