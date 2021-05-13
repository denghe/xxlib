#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct Peer;

// 继承默认监听器覆盖关键函数
struct Listener : EP::TcpListener<Peer> {
    using EP::TcpListener<Peer>::TcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<Peer> const& peer) override;
};
