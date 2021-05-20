#include <coroutine>
#include <exception>
#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_set>

template<typename...TS>
struct Coro_ {
    struct promise_type {
        Coro_ get_return_object() {
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

    // std::pair<std::coroutine_handle<typename Coro_<TS...>::promise_type>, std::tuple<TS...>*>
    using KV = std::pair<std::coroutine_handle<promise_type>, std::tuple<TS...>*>;
};


// co_yield Cond().Xxxxx(v).Xxxx(v)...;
struct Cond {
    Cond() = default;
    Cond(Cond const&) = delete;
    Cond& operator=(Cond const&) = delete;
    Cond(Cond &&) = default;
    Cond& operator=(Cond &&) = default;

    union {
        uint32_t flags = 0;
        struct {
            uint8_t hasSleepTimes;
            uint8_t hasSleepSeconds;
            // more
        };
    };

    int _sleepTimes = 0;
    double _sleepSeconds = 0.0;
    // more

    Cond&& SleepTimes(int const& v) && {
        hasSleepTimes = true;
        _sleepTimes = v;
        return std::move(*this);
    }
    Cond&& SleepSeconds(int const& v) && {
        hasSleepSeconds = true;
        _sleepSeconds = v;
        return std::move(*this);
    }
    // more
};

namespace std {
    template<typename...TS>
    struct hash<std::pair<std::coroutine_handle<typename Coro_<TS...>::promise_type>, std::tuple<TS...>*>> {
        size_t operator()(Coro_<TS...> const& v) const {
            static_assert(sizeof(v) == sizeof(size_t) * 2);
            return (((size_t*)&v)[0] / sizeof(size_t)) ^(((size_t*)&v)[1] / sizeof(size_t));
        }
    };
}

struct Coros {
    Coros() = default;
    Coros(Coros const&) = delete;
    Coros& operator=(Coros const&) = delete;
    Coros(Coros &&) = default;
    Coros& operator=(Coros &&) = default;

    using C = Coro_<Cond>;

    using KV = C::KV;
    std::unordered_set<KV> cs;
    std::unordered_set<KV> cs_int;
    std::unordered_set<KV> cs_double;

    operator bool() const {
        return !cs_int.empty() && !cs_double.empty();
    }

    // c: Coro Func(...) { ... co_yield ?; ... co_return
    void Add(C &&c) {
        if (c.h.done()) return;
        auto&& kv = cs.insert(KV{c.h, &c.h.promise().v});
        auto&& cond = std::get<Cond>(*kv.second);
        if (cond.hasSleepTimes) {
            cs_int.emplace(kv);
        }
        if (cond.hasSleepSeconds) {
            //
        }
        // todo
    }

//    void Handle(KV const& kv) {
//
//    }

    // execute all 1 times
    void operator()() {
        // 倒扫以方便交换删除
//        for (auto &&i = cs.size() - 1; i != (size_t) -1; --i) {
//            auto &c = cs[i];
//            c.first.resume();
//            if (c.first.done()) {
//                if (i < cs.size() - 1) {
//                    std::swap(cs[cs.size() - 1], cs[i]);
//                }
//                cs.pop_back();
//            }
//        }
    }

    ~Coros() {
//        for (auto &&c : cs) {
//            c.first.destroy();
//        }
    }
};

using Coro = Coros::C;

Coro test() {
    for (int i = 3; i <= 4; ++i) {
        std::cout << i << std::endl;
        co_yield Cond().SleepTimes(i);
    }
}


int main() {
    Coros cs;
    cs.Add(test());
    int n = 0;
    while (cs) {
        cs();
        std::cout << "------------------- " << (++n) << " -------------------" << std::endl;
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

