#pragma once

// faster than task 1/4 but does not support call sub coro & return value
// important: only support static function or lambda !!!  COPY data from arguments !!! do not ref !!!

#include "xx_typetraits.h"

namespace xx {
    template<typename R> struct CoroBase_promise_type { R y; };
    template<> struct CoroBase_promise_type<void> {};
    template<typename R>
    struct Coro_ {
        struct promise_type : CoroBase_promise_type<R> {
            Coro_ get_return_object() { return { H::from_promise(*this) }; }
            std::suspend_always initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept(true) { return {}; }
            template<typename U>
            std::suspend_always yield_value(U&& v) { if constexpr(!std::is_void_v<R>) this->y = std::forward<U>(v); return {}; }
            void return_void() {}
            void unhandled_exception() { std::rethrow_exception(std::current_exception()); }
        };
        using H = std::coroutine_handle<promise_type>;
        H h;
        Coro_() : h(nullptr) {}
        Coro_(H h) : h(h) {}
        ~Coro_() { if (h) h.destroy(); }
        Coro_(Coro_ const& o) = delete;
        Coro_& operator=(Coro_ const&) = delete;
        Coro_(Coro_&& o) noexcept : h(o.h) { o.h = nullptr; };
        Coro_& operator=(Coro_&& o) noexcept { std::swap(h, o.h); return *this; };

        void Run() {
            while (!h.done()) {
                h.resume();
            }
        }
        void operator()() { h.resume(); }
        operator bool() const { return h.done(); }
        bool Resume() { h.resume(); return h.done(); }
    };
    using Coro = Coro_<void>;

    template<>
    struct IsPod<Coro> : std::true_type {};
    template<typename T>
    struct IsPod<Coro_<T>> : IsPod<T> {};

//#define CoYield co_yield std::monostate()
#define CoReturn co_return
#define CoAwait( coType ) { auto&& c = coType; while(!c) { CoYield; c(); } }
#define CoSleep( duration ) { auto tp = std::chrono::steady_clock::now() + duration; do { CoYield; } while (std::chrono::steady_clock::now() < tp); }

}

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