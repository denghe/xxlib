#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct PingTimer : EP::Timer {
    using EP::Timer::Timer;

    const double intervalSeconds = 3;
    void Start();

    void Timeout() override;
};
