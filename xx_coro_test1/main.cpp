#include <iostream>
#include <atomic>
#include <chrono>
using namespace std::chrono_literals;
#include "xx_coro.h"
#include "xx_threadpool.h"

xx::ThreadPool<> tp;

auto DoSomeTask() {
    auto ok = std::make_shared<bool>(false);
    tp.Add([ok_w = std::weak_ptr<bool>(ok)]{
        std::this_thread::sleep_for(2s);
        if (auto ok = ok_w.lock()) {
            *ok = true;
            std::cout << "ok = true" << std::endl;
        }
        else {
            std::cout << "coroutine is dead" << std::endl;
        }
    });
    return ok;
}

xx::Coro Test() {
    auto ok = DoSomeTask();
    for (int i = 3; i <= 4; ++i) {
        std::cout << i << std::endl;
        co_yield i;
    }
    co_yield xx::Cond(2).UpdateCallback([&]{
        std::cout << "ok == " << *ok << std::endl;
        return *ok;
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
    while(tp) {
        std::this_thread::sleep_for(100ms);
    }
    return 0;
}
