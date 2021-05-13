#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct APeer;

// 继承默认监听器覆盖关键函数
struct Listener : EP::TcpListener<APeer> {
    using EP::TcpListener<APeer>::TcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<APeer> const& peer) override;
};
