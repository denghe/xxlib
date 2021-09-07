#pragma once
#include "xx_epoll_kcp.h"
#include "peer.h"
namespace EP = xx::Epoll;

// 继承默认监听器覆盖关键函数
struct Listener : EP::KcpListener<Peer> {
    using EP::KcpListener<Peer>::KcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<Peer> const& peer) override;
};
