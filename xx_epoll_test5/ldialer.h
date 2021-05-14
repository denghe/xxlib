#pragma once
#include "xx_epoll.h"
#include "lpeer.h"
namespace EP = xx::Epoll;

// 连到 db 的拨号器
struct LDialer : EP::TcpDialer<LPeer> {
    using EP::TcpDialer<LPeer>::TcpDialer;

    void Connect(xx::Shared<LPeer> const &peer) override;
};
