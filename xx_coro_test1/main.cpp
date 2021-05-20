#include <coroutine>
#include <exception>
#include <iostream>
#include <vector>
#include <tuple>

template<typename...TS>
struct Coro {
    struct promise_type {
        Coro get_return_object() {
            return {.h = std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }

        std::exception_ptr e;
        void unhandled_exception() {
            e = std::current_exception();
        }

        std::tuple<TS...> v;
        template<typename U>
        std::suspend_always yield_value(U &&u) {
            if constexpr(std::is_same_v<std::tuple<TS...>, std::decay_t<U>>) {
                v = std::forward<U>(u);
            } else {
                std::get<std::decay_t<U>>(v) = std::forward<U>(u);
            }
            return {};
        }
    };

    std::coroutine_handle<promise_type> h;

    using KV = std::pair<std::coroutine_handle<promise_type>, promise_type *>;
};


template<typename...TS>
struct Coros {
    Coros() = default;
    Coros(Coros const&) = delete;
    Coros& operator=(Coros const&) = delete;
    Coros(Coros &&) = default;
    Coros& operator=(Coros &&) = default;

    using C = Coro<TS...>;
    using KV = typename C::KV;
    std::vector<KV> cs;

    operator bool() const {
        return !cs.empty();
    }

    // c: Coro<?,?,?...> Func(...) { ... co_yield ?; ... co_return
    void Add(C &&c) {
        if (!c.h.done()) {
            cs.emplace_back(c.h, &c.h.promise());
        }
    }

    // execute all 1 times
    void operator()() {
        // 倒扫以方便交换删除
        for (auto &&i = cs.size() - 1; i != (size_t) -1; --i) {
            auto &c = cs[i];
            c.first.resume();
            if (c.first.done()) {
                if (i < cs.size() - 1) {
                    std::swap(cs[cs.size() - 1], cs[i]);
                }
                cs.pop_back();
            }
        }
    }

    ~Coros() {
        for (auto &&c : cs) {
            c.first.destroy();
        }
    }
};

Coro<int> test() {
    for (int i = 0; i < 3; ++i) {
        std::cout << i << std::endl;
        co_yield i;
    }
}

// sleep version
struct CoroSleep : std::tuple<int> {};

struct CorosEx : Coros<int> {

};

int main() {
    Coros<int> cs;
    cs.Add(test());
    while (cs) {
        cs();
    }
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

