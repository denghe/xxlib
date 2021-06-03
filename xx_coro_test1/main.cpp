#include <iostream>
#include <atomic>
#include <chrono>
using namespace std::chrono_literals;
#include "xx_coro.h"
#include "xx_threadpool.h"

xx::ThreadPool<> tp(10);

template<typename F>
std::shared_ptr<bool> NewTask(F&& f) {
    auto ok = std::make_shared<bool>(false);
    tp.Add([ok_w = std::weak_ptr<bool>(ok), f = std::forward<F>(f)]{
        f();
        if (auto ok = ok_w.lock()) {
            assert(!*ok);
            *ok = true;
        }
    });
    return ok;
}

xx::Coro Test() {
    std::cout << "Begin" << std::endl;
    {
        auto r1 = NewTask([] { std::this_thread::sleep_for(0.3s); });
        auto r2 = NewTask([] { std::this_thread::sleep_for(0.4s); });
        auto r3 = NewTask([] { std::this_thread::sleep_for(0.5s); });
        int n = 0;
        co_yield xx::Cond(1).Wait(n, r1, r2, r3);
        if (n == 3) {
            std::cout << "all tasks are completed" << std::endl;
        }
    }

    for (int i = 1; i <= 2; ++i) {
        std::cout << "sleep " << i << " seconds" << std::endl;
        co_yield i;
    }

    std::cout << "while true" << std::endl;
    while(true) {
        co_yield 0;
        if (true) break;
    }

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
    tp.Join();
    return 0;
}
