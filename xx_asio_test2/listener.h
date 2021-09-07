#pragma once
#include "xx_epoll_kcp.h"
#include "cpeer.h"
namespace EP = xx::Epoll;

// 继承默认监听器覆盖关键函数
struct Listener : EP::KcpListener<CPeer> {
    using EP::KcpListener<CPeer>::KcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<CPeer> const& peer) override;
};
