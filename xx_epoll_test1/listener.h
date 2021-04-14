#pragma once
#include "xx_epoll.h"
#include "cpeer.h"
namespace EP = xx::Epoll;

// 预声明
struct Server;

// 继承默认监听器覆盖关键函数
struct Listener : EP::TcpListener<CPeer> {
    using EP::TcpListener<CPeer>::TcpListener;

    // 拿到 server 上下文引用, 以方便写事件处理代码
    Server &GetServer();

    // 连接已建立, 搞事
    void Accept(xx::Shared<CPeer> const& peer) override;
};
