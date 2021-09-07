#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct TaskTimer : EP::Timer {
    using EP::Timer::Timer;

    const double intervalSeconds = 1;
    void Start();

    void Timeout() override;
};
