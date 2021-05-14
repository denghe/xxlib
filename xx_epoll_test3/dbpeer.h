#pragma once
#include "peer.h"

struct DBPeer : Peer {
    using Peer::Peer;

    // cleanup callbacks, DelayUnhold, reset server.dbPeer
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&ob) override;

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) override;
};
