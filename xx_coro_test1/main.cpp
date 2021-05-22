#include <iostream>
#include <atomic>
#include <chrono>
using namespace std::chrono_literals;
#include "xx_coro.h"
#include "xx_threadpool.h"

xx::ThreadPool<> tp;

xx::Coro Test() {
    std::atomic<bool> ok = false;
    tp.Add([&]{
        std::this_thread::sleep_for(1s);
        ok = true;
        std::cout << "ok = true" << std::endl;
    });
    for (int i = 3; i <= 4; ++i) {
        std::cout << i << std::endl;
        co_yield i;
    }
    co_yield xx::Cond(50).UpdateCallback([&]{
        auto rtv = (bool)ok;
        std::cout << "ok == " << rtv << std::endl;
        return rtv;
    });
    std::cout << "End" << std::endl;
}

xx::Coro Test2() {
    CoAwait(Test());
}

int main() {
    xx::Coros cs;
    cs.Add(Test2());
    while (cs) {
        std::cout << "------------------- " << cs.cursor << " -------------------" << std::endl;
        cs();
        std::this_thread::sleep_for(100ms);
    }
    return 0;
}
