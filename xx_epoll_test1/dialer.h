#pragma once
#include "xx_epoll.h"
#include "speer.h"
namespace EP = xx::Epoll;

// 预声明
struct Server;

// 连到 lobby, game servers 的拨号器
struct Dialer : EP::TcpDialer<SPeer> {
    using EP::TcpDialer<SPeer>::TcpDialer;

    // 配置信息. 连接成功后传导给 peer
    int serverId = -1;

    // 拿到 server 上下文引用, 以方便写事件处理代码
    Server &GetServer();

    // 连接已建立, 继续与目标服务协商？
    void Connect(xx::Shared<SPeer> const &peer) override;
};
