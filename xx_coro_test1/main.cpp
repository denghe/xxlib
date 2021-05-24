#include <iostream>
#include <atomic>
#include <chrono>
using namespace std::chrono_literals;
#include "xx_coro.h"
#include "xx_threadpool.h"

xx::ThreadPool<> tp;

std::shared_ptr<bool> DoSomeTask(std::string && tips) {
    auto ok = xx::Cond::MakeOK();
    tp.Add([ok_w = std::weak_ptr<bool>(ok), tips = std::move(tips)]{
        // do some work
        std::this_thread::sleep_for(0.5s);
        // ensure
        if (auto ok = ok_w.lock()) {
            *ok = true;
            std::cout << tips << ": set ok" << std::endl;
        }
        else {
            std::cout << tips << ": coroutine is dead" << std::endl;
        }
    });
    return ok;
}

xx::Coro Test() {
    auto ok1 = DoSomeTask("task1");
    auto ok2 = DoSomeTask("task2");
    co_yield xx::Cond(10).WaitOK(ok1, ok2);

    for (int i = 1; i <= 2; ++i) {
        std::cout << "i = " << i << std::endl;
        co_yield i;
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
    while(tp) {
        std::this_thread::sleep_for(100ms);
    }
    return 0;
}
