#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct GPeer;

// 继承默认监听器覆盖关键函数
struct GListener : EP::TcpListener<GPeer> {
    using EP::TcpListener<GPeer>::TcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<GPeer> const& peer) override;
};
