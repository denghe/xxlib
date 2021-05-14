﻿#pragma once
#include "xx_epoll.h"
#include "xx_dict_mk.h"
#include "xx_obj.h"
namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct GPeer;
struct VPeer;
struct PingTimer;
struct DBDialer;
struct DBPeer;
struct Game;

//struct SPeer

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 等待 server 接入的监听器
    xx::Shared<Listener> listener;

    // 用于 ping 内部服务的 timer
    xx::Shared<PingTimer> pingTimer;

    // for connect to db server
    xx::Shared<DBDialer> dbDialer;
    xx::Shared<DBPeer> dbPeer;

    // shared obj manager
    xx::ObjManager om;

//    // server peers. key: server id
//    std::unordered_map<uint32_t, xx::Shared<SPeer>> sps;

    // key: gameId
    std::unordered_map<int, xx::Shared<Game>> games;

    // gateway peers. key: gateway id
    std::unordered_map<uint32_t, xx::Shared<GPeer>> gps;

    // virtual peers( players ). key0: gateway id << 32 | client id   key1: account id (logic data)
    xx::DictMK<xx::Shared<VPeer>, uint64_t, int32_t> vps;

    // Make: accountId = --server->autoDecId
    // Kick: clientId = --server->autoDecId
    int32_t autoDecId = 0;

    // 根据 config 进一步初始化各种成员
    int Init();

    // logic here( call vps's update )
    int FrameUpdate() override;

    // 得到执行情况的快照
    std::string GetInfo();
};
