#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

// 预声明
struct Listener;
struct CPeer;
struct TaskTimer;

// 服务本体
struct Server : EP::Context {
    // 继承构造函数
    using EP::Context::Context;

    // 客户端连接id 自增量, 产生 peer 时++填充
    uint32_t cpeerAutoId = 0;

    // 等待 client 接入的监听器
    xx::Shared<Listener> listener;

    // 用于计划任务的 timer
    xx::Shared<TaskTimer> taskTimer;

    // client peers
    std::unordered_map<uint32_t, xx::Shared<CPeer>> cps;

    // 根据 config 进一步初始化各种成员
    int Init();
};
