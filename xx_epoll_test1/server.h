#pragma once
#include "xx_epoll.h"
#include <tsl/hopscotch_map.h>
namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct Dialer;
struct CPeer;
struct SPeer;
struct PingTimer;
struct TaskTimer;

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 客户端连接id 自增量, 产生 peer 时++填充
    uint32_t cpeerAutoId = 0;

    // 等待 client 接入的监听器
    xx::Shared<Listener> listener;

    // 用于 ping 内部服务的 timer
    xx::Shared<PingTimer> pingTimer;

    // 用于计划任务的 timer
    xx::Shared<TaskTimer> taskTimer;

    // client peers
    tsl::hopscotch_map<uint32_t, xx::Shared<CPeer>> cps;

    // dialer + peer s
    tsl::hopscotch_map<uint32_t, std::pair<xx::Shared<Dialer>, xx::Shared<SPeer>>> dps;


    // 根据 config 进一步初始化各种成员
    int Init();

    // 得到执行情况的快照
    std::string GetInfo();
};
