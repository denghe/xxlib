﻿#include "xx_helpers.h"
#include "xx_string.h"
int main() {
    xx::Data d;
    auto t = xx::NowEpochSeconds();
    d.Reserve(4000000000);
    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
    std::cin.get();
    t = xx::NowEpochSeconds();
    for (size_t i = 0; i < 1000000000; i++) {
        d.WriteFixed((uint32_t)123);
    }
    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
    xx::CoutN("d.len = ", d.len, "d.cap = ", d.cap);
    std::cin.get();
    return 0;
}
