#pragma once
#include "xx_epoll.h"
#include <tsl/hopscotch_map.h>
#include "xx_dict_mk.h"
namespace EP = xx::Epoll;

// 预声明
struct Listener;
//struct GPeer;
//struct SPeer;
//struct VPeer;
struct PingTimer;
// Lobby? Room?

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 等待 server 接入的监听器
    xx::Shared<Listener> listener;

    // 用于 ping 内部服务的 timer
    xx::Shared<PingTimer> pingTimer;

//    // server peers. key: server id
//    tsl::hopscotch_map<uint32_t, xx::Shared<SPeer>> sps;
//
//    // gateway peers. key: gateway id
//    tsl::hopscotch_map<uint32_t, xx::Shared<GPeer>> gps;
//
//    // virtual peers. key1: account id  key2: gateway id + client id
//    xx::DictMK<xx::Shared<VPeer>, int32_t, uint64_t> vps;

    // auto decrease account id generator for guest player key1 in the vps
    // after login, update key1 to real account id
    int32_t guestAccountId = -1;

    // 根据 config 进一步初始化各种成员
    int Init();

    // 得到执行情况的快照
    std::string GetInfo();
};
