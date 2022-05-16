#pragma once

// 协程的精简实现. 用法：co_yield 0. 达到暂停执行的目的. 协程函数返回值为 xx::Coro. 嵌套调用使用 CoAwait 宏

// 已知问题: 如果用 lambda 带捕获列表 返回 Generator<> 就会出问题。捕获列表中的数据似乎被破坏了
// 所以确保 [] 为空, 直接通过传参来替代捕获

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#else
static_assert(false, "No co_await support");
#endif
#include <cassert>
#include <variant>
#include <memory>
#include <utility>
#include <vector>
#include <chrono>
using namespace std::literals;
using namespace std::literals::chrono_literals;

namespace xx {
#if __has_include(<coroutine>)
    using namespace std;
#else
    using namespace std::experimental;
    using namespace cxx14;
#endif

    struct Coro {
        struct promise_type;
        using handle_type = std::coroutine_handle<promise_type>;
        struct promise_type {
            auto get_return_object() { return Coro(handle_type::from_promise(*this)); }
            suspend_never initial_suspend() { return {}; }
            suspend_always final_suspend() noexcept(true) { return {}; }
            suspend_always yield_value(int v) { return {}; }
            void return_void() {}
            void unhandled_exception() {}
        };

        Coro(handle_type h) : h(h) {}
        ~Coro() { if (h) h.destroy(); }
        Coro(Coro const& o) = delete;
        Coro& operator=(Coro const&) = delete;
        Coro(Coro&& o) noexcept : h(o.h) { o.h = nullptr; };
        Coro& operator=(Coro&&) = delete;

        void operator()() { h.resume(); }
        operator bool() { return h.done(); }

    private:
        handle_type h;
    };

    typedef std::aligned_storage_t<sizeof(Coro), alignof(Coro)> CoroStore;

    struct Coros {
        Coros() = default;
        Coros(Coros const&) = delete;
        Coros& operator=(Coros const&) = delete;
        Coros(Coros&&) = default;
        Coros& operator=(Coros&&) = default;

        std::vector<CoroStore> coros;

        ~Coros() {
            Clear();
        }

        void Add(Coro&& g) {
            if (g) return;
            new (&coros.emplace_back()) Coro(std::move(g));
        }

        void Clear() {
            for (auto& o : coros) {
                reinterpret_cast<Coro&>(o).~Coro();
            }
            coros.clear();
        }

        operator bool() const {
            return !coros.empty();
        }

        void operator()() {
            for (int i = (int)coros.size() - 1; i >= 0; --i) {
                auto& c = reinterpret_cast<Coro&>(coros[i]);
                if (c(); c) {
                    c.~Coro();
                    coros[i] = coros[coros.size() - 1];
                    coros.pop_back();
                }
            }
        }
    };

}

#define CoYield co_yield 0
#define CoAwait(g_) { auto&& g = std::move(g); while(!g.Done()) { CoYield; g.Resume(); } }
#define CoSleep(d) { auto tp = std::chrono::steady_clock::now() + d; do { CoYield; } while (std::chrono::steady_clock::now() < tp); }


/*

example:

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


*/