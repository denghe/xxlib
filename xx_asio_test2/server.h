﻿#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct Peer;

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 客户端连接id 自增量, 产生 peer 时++填充
    uint32_t pid = 0;

    // 等待 client 接入的监听器
    xx::Shared<Listener> listener;

    // client peers
    tsl::hopscotch_map<uint32_t, xx::Shared<Peer>> ps;

    // init members
    int Init();

    // game logic here
    int FrameUpdate() override;
};