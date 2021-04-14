#pragma once
#include "xx_epoll.h"
#include "speer.h"
namespace EP = xx::Epoll;

// 连到 lobby, game servers 的拨号器
struct Dialer : EP::TcpDialer<SPeer> {
    using EP::TcpDialer<SPeer>::TcpDialer;

    // 配置信息. 连接成功后传导给 peer
    uint32_t serverId = 0xFFFFFFFFu;

    // 连接已建立, 继续与目标服务协商？
    void Connect(xx::Shared<SPeer> const &peer) override;
};
