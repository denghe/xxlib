#pragma once

// important: only support static function or lambda !!!

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

namespace cxx14 {}
namespace xx {
    using namespace std;
#if __has_include(<coroutine>)
#else
    using namespace std::experimental;
    using namespace cxx14;
#endif

    struct Coro {
        struct promise_type;
        using handle_type = coroutine_handle<promise_type>;
        struct promise_type {
            auto get_return_object() { return Coro(handle_type::from_promise(*this)); }
            suspend_never initial_suspend() { return {}; }
            suspend_always final_suspend() noexcept(true) { return {}; }
            suspend_always yield_value(int v) { return {}; }
            void return_void() {}
            void unhandled_exception() {
                std::rethrow_exception(std::current_exception());
            }
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

#define CoType xx::Coro
#define CoYield co_yield 0
#define CoAwait( coType ) { auto&& c = coType; while(!c) { CoYield; c(); } }
#define CoSleep( duration ) { auto tp = std::chrono::steady_clock::now() + duration; do { CoYield; } while (std::chrono::steady_clock::now() < tp); }

/*

example:

#include "xx_coro_simple.h"
#include <thread>
#include <iostream>

CoType func1() {
    CoSleep(1s);
}
CoType func2() {
    CoAwait( func1() );
    std::cout << "func 1 out" << std::endl;
    CoAwait( func1() );
    std::cout << "func 1 out" << std::endl;
}

int main() {
    xx::Coros cs;

    auto func = [](int b, int e)->CoType {
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
    cs.Add([]()->CoType {
        CoSleep(1s);
        std::cout << "CoSleep 1s" << std::endl;
    }());
    cs.Add(func2());

LabLoop:
    cs();
    if (cs) {
        std::this_thread::sleep_for(500ms);
        goto LabLoop;
    }

    return 0;
}

*/