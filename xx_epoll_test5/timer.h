#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

struct Timer : EP::Timer {
    using EP::Timer::Timer;

    const double intervalSeconds = 1;
    void Start();

    void Timeout() override;
};
