#include "xx_coro_simple.h"
#include <thread>
#include <iostream>

int main() {
    xx::Coros cs;

    auto func = [](int b, int e)->xx::Coro {
        CoYield;
        for (size_t i = b; i <= e; i++) {
            std::cout << i << std::endl;
            CoYield;
        }
        std::cout << b << "-" << e << " end" << std::endl;
    };

    cs.Add(func(1, 5));
    cs.Add(func(6, 10));
    cs.Add(func(11, 15));
    cs.Add([]()->xx::Coro {
        CoSleep(3s);
        std::cout << "CoSleep 3s" << std::endl;
    }());

LabLoop:
    cs();
    if (cs) {
        std::this_thread::sleep_for(500ms);
        goto LabLoop;
    }

    return 0;
}
