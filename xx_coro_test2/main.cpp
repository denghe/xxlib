#include <iostream>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

#include "xx_coro.h"
#include "xx_threadpool.h"

struct dummy { // Awaitable
    std::suspend_always operator co_await(){ return {}; }
};

struct HelloWorldCoro {
    struct promise_type {
        HelloWorldCoro get_return_object() { return this; }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() {}

        std::suspend_always await_transform(const dummy &) { return {}; }
    };

    HelloWorldCoro(promise_type *p) : m_handle(std::coroutine_handle<promise_type>::from_promise(*p)) {}

    ~HelloWorldCoro() { m_handle.destroy(); }

    std::coroutine_handle<promise_type> m_handle;
};

HelloWorldCoro print_hello_world() {
    std::cout << "Hello ";
    co_await dummy{};
    std::cout << "World!" << std::endl;
}


template<typename T>
struct task {
    struct promise_type {
        std::coroutine_handle<> precursor;
        T data;

        task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() const noexcept { return {}; }
        void unhandled_exception() {}
        auto final_suspend() const noexcept {
            struct awaiter {
                bool await_ready() const noexcept { return false; }
                void await_resume() const noexcept {}
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    if (auto&& pc = h.promise().precursor) return pc;
                    return std::noop_coroutine();
                }
            };
            return awaiter{};
        }
        template<std::convertible_to<T> From> void return_value(From&& value) noexcept { data = std::forward<From>(value); }
    };

    bool await_ready() const noexcept { return handle.done(); }
    T await_resume() const noexcept { return std::move(handle.promise().data); }
    void await_suspend(std::coroutine_handle<> coroutine) const noexcept { handle.promise().precursor = coroutine; }

    ~task() { handle.destroy(); }
    std::coroutine_handle<promise_type> handle;
};

task<int> test1(int rtv) {
    std::cout << 1 << std::endl;
    co_await std::suspend_always();
    std::cout << 2 << std::endl;
    co_return rtv;
}
task<int> test2() {
    std::cout << 3 << std::endl;
    auto x = co_await test1(123);
    std::cout << 4 << std::endl;
    std::cout << x << std::endl;
    co_return x;
}

int main() {
//    HelloWorldCoro mycoro = print_hello_world();
//    mycoro.m_handle.resume();
//    return EXIT_SUCCESS;

    auto t = test2();
    //t.handle.resume();

    return EXIT_SUCCESS;
}
