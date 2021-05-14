#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct SPeer;

// 继承默认监听器覆盖关键函数
struct SListener : EP::TcpListener<SPeer> {
    using EP::TcpListener<SPeer>::TcpListener;

    // 连接已建立, 搞事
    void Accept(xx::Shared<SPeer> const& peer) override;
};
