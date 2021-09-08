#pragma once
#include "xx_epoll.h"
#include "xx_obj.h"
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

    // package serde
    xx::ObjManager om;

    // for timer
    int64_t lastMS = 0;

    // for send Obj
    xx::Data tmp;

    // game context
    xx::ObjBase_s gctx;

    // init members
    int Init();

    // game logic here
    int FrameUpdate() override;
};
