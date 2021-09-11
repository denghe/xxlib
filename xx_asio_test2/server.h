#pragma once

#include "xx_epoll.h"
#include "xx_obj.h"
#include "ss.h"

namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct Peer;

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // singleton usage
    inline static Server *instance = nullptr;

    // 客户端连接id 自增量, 产生 peer 时++填充
    uint32_t pid = 0;

    // 等待 client 接入的监听器
    xx::Shared<Listener> listener;

    // client peers
    tsl::hopscotch_map<uint32_t, xx::Shared<Peer>> ps;

    // package serde
    xx::ObjManager om;

    // for timer
    double totalDelta = 0;
    int64_t lastMS = 0;

    // for peer timeout
    double peerTimeoutSecs = 15;

    // for peer.Send
    xx::Data tmp;

    // game context
    xx::Shared<SS::Scene> scene;

    // package cache
    xx::Shared<SS_S2C::Sync> sync;
    xx::Data syncData;
    xx::Shared<SS_S2C::Event> event;
    xx::Data eventData;
    xx::Shared<SS_S2C::EnterResult> enterResult;

    // current frame c2s cache
    tsl::hopscotch_map<uint32_t, xx::Shared<SS_C2S::Enter>> newEnters;

    // fill send data to d ( contains len header )
    void WriteTo(xx::Data &d, xx::ObjBase_s const &o);

    // return 0: handled. -1: error. kick peer?
    int HandlePackage(Peer &p, xx::ObjBase_s &o);

    // init members
    int Init();

    // game logic here
    int FrameUpdate() override;

    // call by listener accept. return 0: success
    int Accept(xx::Shared<Peer> const &p);

    // call by peer close
    void Kick(uint32_t const &clientId);
};
