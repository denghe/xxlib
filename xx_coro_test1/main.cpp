#include "xx_coro.h"
#include <iostream>

template<typename T = int>
struct co_rtv {
    struct promise_type {
        T v;
        std::exception_ptr e;

        co_rtv get_return_object() {
            return {.h = std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_never final_suspend() noexcept { return {}; }

        void unhandled_exception() {
            e = std::current_exception();
        }

        template<typename U>
        std::suspend_always yield_value(U &&value) {
            v = std::forward<U>(value);
            return {};
        }
    };

    std::coroutine_handle<promise_type> h;
};

co_rtv<> test() {
    for (int i = 0;; ++i)
        co_yield i;
        //co_await std::suspend_always{};
}

int main() {
    auto h = test().h;
    auto &promise = h.promise();
    for (int i = 0; i < 3; ++i) {
        std::cout << promise.v << std::endl;
        h();
    }
    h.destroy();
    return 0;
}


//
//CoRtv Delay(int n) {
//    while(n--) CoYield;
//}
//
//CoRtv Test(int id) {
//    CoAwait(Delay(1));
//    std::cout << id << std::endl;
//    CoAwait(Delay(2));
//    std::cout << id << std::endl;
//}
//
//int main() {
//    xx::Coros coros;
//    //coros.Add()
//	return 0;
//}
