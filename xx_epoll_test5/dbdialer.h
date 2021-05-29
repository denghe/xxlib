#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct DBPeer;

// 连到 db 的拨号器
struct DBDialer : EP::TcpDialer<DBPeer> {
    using EP::TcpDialer<DBPeer>::TcpDialer;

    void Connect(xx::Shared<DBPeer> const &peer) override;
};
