#pragma once

#include <exception>
#include <tuple>
#include <functional>
#include <unordered_set>
#include <memory>
#include "xx_nodepool.h"


// for clion editor read line error
#define XX_COROUTINE_CLANG_SUPPORT 0

#if XX_COROUTINE_CLANG_SUPPORT && defined(__clang__)
#include <experimental/coroutine>
#else

#include <coroutine>

#endif

#define CoAwait(func) {auto&& g = func; while(!g.Resume()) { co_yield g.Value(); }}

namespace xx {
#if XX_COROUTINE_CLANG_SUPPORT && defined(__clang__)
    using namespace std::experimental;
#else
    using namespace std;
#endif

    template<typename T>
    struct Generator {
        struct promise_type;
        using Handle = coroutine_handle<promise_type>;
    private:
        Handle h;
    public:
        explicit Generator(Handle &&h) : h(std::move(h)) {}

        Generator(const Generator &) = delete;

        Generator &operator=(Generator const &) = delete;

        // can't use == default here
        Generator(Generator &&o) noexcept {
            h = o.h;
            o.h = {};
        };

        // can't use == default here
        Generator &operator=(Generator &&o) noexcept {
            h = o.h;
            o.h = {};
            return *this;
        }

        ~Generator() {
            if (h) {
                h.destroy();
            }
        }

        bool Resume() {
            h();
            if (h.promise().e)
                std::rethrow_exception(h.promise().e);
            return h.done();
        }

        bool Done() {
            return h.done();
        }

        [[nodiscard]] T const &Value() const {
            return h.promise().v;
        }

        T &Value() {
            return h.promise().v;
        }

        struct promise_type {
            T v;
            std::exception_ptr e;

            promise_type() = default;

            ~promise_type() = default;

            promise_type(promise_type const &) = delete;

            promise_type(promise_type &&) = delete;

            promise_type &operator=(promise_type const &) = delete;

            promise_type &operator=(promise_type &&) = delete;

            [[maybe_unused]] auto initial_suspend() {
                return suspend_always{};
            }

            [[maybe_unused]] auto final_suspend() noexcept {
                return suspend_always{};
            }

            [[maybe_unused]] auto get_return_object() {
                return Generator{Handle::from_promise(*this)};
            }

            [[maybe_unused]] void return_void() {}

            template<std::convertible_to<T> From>
            [[maybe_unused]] auto yield_value(From &&some_value) {
                v = std::forward<From>(some_value);
                return suspend_always{};
            }

            [[maybe_unused]] void unhandled_exception() {
                e = std::current_exception();
            }
        };
    };


    // co_yield Cond().Xxxxx(v).Xxxx(v)...;
    struct Cond {
        Cond(Cond const &) = default;

        Cond &operator=(Cond const &) = default;

        Cond(Cond &&) = default;

        Cond &operator=(Cond &&) = default;

        union {
            uint64_t flags = 0;
            struct {
                uint8_t hasSleepTimes;
                uint8_t hasSleepSeconds;
                uint8_t hasUpdateCallback;
//                uint8_t hasEventCallback;
                // more
            };
        };

        int sleepTimes = 0;
        double sleepSeconds = 0.0;
        std::function<bool()> updateCallback;
//        std::function<int(void*)> eventCallback;
        // more

        Cond() = default;

        Cond(int const &sleepTimes_) {
            SleepTimes(sleepTimes_);
        }

        Cond(double const &sleepSeconds_) {
            SleepSeconds(sleepSeconds_);
        }

        Cond &&SleepTimes(int const &v) {
            hasSleepTimes = true;
            sleepTimes = v;
            return std::move(*this);
        }

        Cond &&SleepSeconds(double const &v) {
            hasSleepSeconds = true;
            sleepSeconds = v;
            return std::move(*this);
        }

        template<typename F, class ENABLED = std::enable_if_t<!std::is_arithmetic_v<F>>>
        Cond &&UpdateCallback(F &&cbFunc) {
            hasUpdateCallback = true;
            updateCallback = std::forward<F>(cbFunc);
            return std::move(*this);
        }

//        template<typename F, class ENABLED = std::enable_if_t<!std::is_arithmetic_v<F>>>
//        [[maybe_unused]] Cond &&EventCallback(F &&cbFunc) {
//            hasEventCallback = true;
//            eventCallback = std::forward<F>(cbFunc);
//            return std::move(*this);
//        }

    protected:
        template<std::size_t...Idxs, typename T>
        static int Wait_(std::index_sequence<Idxs...>, T const &t) {
            return (... + (**std::get<Idxs>(t) ? 1 : 0));
        }

    public:

        template<typename...OKS>
        [[maybe_unused]] Cond &&Wait(OKS &...oks) {
            static_assert(sizeof...(OKS) > 0);
            return UpdateCallback([t = std::make_tuple(&oks...)] {
                return Wait_(std::make_index_sequence<sizeof...(OKS)>(), t) == sizeof...(OKS);
            });
        }
    };

    using Coro = Generator<Cond>;

    struct Coros {
        Coros(Coros const &) = delete;

        Coros &operator=(Coros const &) = delete;

        Coros(Coros &&) = default;

        Coros &operator=(Coros &&) = default;

        // 1: wheel index    2: update prev   3: update next
        NodePool<std::tuple<Generator<Cond>, int, int, int>> nodes;

        int wheelLen;
        int *wheel;
        int cursor = 0;
        double frameDelaySeconds;

        int updateList = -1;
        //std::unordered_set<int> updateCallbacks;
//        std::unordered_set<int> eventCallbacks;

        explicit Coros(double const &framePerSeconds = 10, int const &wheelLen = 10 * 60 * 5)
                : frameDelaySeconds(1.0 / framePerSeconds), wheelLen(wheelLen) {
            wheel = (int *) malloc(wheelLen * sizeof(int));
            memset(wheel, -1, wheelLen * sizeof(int));
        }

        ~Coros() {
            free(wheel);
        }

        void Clear() {
            nodes.Clear();
            memset(wheel, -1, wheelLen * sizeof(int));
            cursor = 0;
            updateList = -1;
            //updateCallbacks.clear();
//            eventCallbacks.clear();
        }

    protected:

        void WheelAdd(int const &idx, int const &wIdx) {
            auto &n = nodes[idx];
            n.prev = -1;
            n.next = wheel[wIdx];
            std::get<1>(n.value) = wIdx;
            if (wheel[wIdx] >= 0) {
                nodes[wheel[wIdx]].prev = idx;
            }
            wheel[wIdx] = idx;
        }

        void WheelRemove(int const &idx) {
            auto &n = nodes[idx];
            assert(n.prev != -2);
            auto &wIdx = std::get<1>(n.value);
            assert(wIdx >= 0);
            if (n.prev < 0 && wheel[wIdx] == idx) {
                wheel[wIdx] = n.next;
            } else {
                nodes[n.prev].next = n.next;
            }
            if (n.next >= 0) {
                nodes[n.next].prev = n.prev;
            }
            wIdx = -1;
        }

        void UpdateAdd(int const &idx) {
            auto &t = nodes[idx].value;
            std::get<2>(t) = -1;            // prev
            std::get<3>(t) = updateList;    // next
            if (updateList != -1) {
                std::get<2>(nodes[updateList].value) = idx;
            }
            updateList = idx;
            //std::cout << __LINE__ << " updateList = " << updateList << std::endl;
        }

        void UpdateRemove(int const &idx) {
            auto &t = nodes[idx].value;
            if (updateList == idx) {
                assert(std::get<2>(t) == -1);
                updateList = std::get<3>(t);
                //std::cout << __LINE__ << " updateList = " << updateList << std::endl;
            } else {
                assert(std::get<2>(t) != -1);
                std::get<3>(nodes[std::get<2>(t)].value) = std::get<3>(t);
            }
            if (std::get<3>(t) != -1) {
                assert(std::get<2>(nodes[std::get<3>(t)].value) == idx);
                std::get<2>(nodes[std::get<3>(t)].value) = std::get<2>(t);
            }
        }

        int CalcSleepTimes(Cond const &c) const {
            int n;
            if (c.hasSleepTimes) {
                assert(c.sleepTimes > 0);
                n = c.sleepTimes;
            } else if (c.hasSleepSeconds) {
                assert(c.sleepSeconds > 0);
                n = (int) (c.sleepSeconds / frameDelaySeconds);
                if (n == 0) {
                    n = 1;
                }
            } else {
                n = 1;
            }
            assert(n < wheelLen);
            return n;
        }

        void HandleCond(Cond &cond, int const &idx, int const &wIdx) {
            WheelAdd(idx, wIdx);
            if (cond.hasUpdateCallback) {
                UpdateAdd(idx);
            }
//            if (cond.hasEventCallback) {
//            }
            // todo: more
        }

        void Resume(int const &idx, Coro &coro, Cond &c) {
            if (coro.Resume()) {
                nodes.Free(idx);
            } else {
                auto num = CalcSleepTimes(c);
                auto wIdx = (cursor + num) % wheelLen;
                HandleCond(c, idx, wIdx);
            }
        }

        void HandleUpdateCallback() {
            if (updateList == -1) return;
            auto idx = updateList;
            do {
                auto &t = nodes[idx].value;
                auto next = std::get<3>(t);
                auto &coro = std::get<0>(t);
                auto &c = coro.Value();
                assert(c.hasUpdateCallback);
                assert(c.updateCallback);
                auto r = c.updateCallback();
                //std::cout << __LINE__ << " r = " << r << std::endl;
                if (r) {
                    UpdateRemove(idx);
                    WheelRemove(idx);
                    Resume(idx, coro, c);
                }
                idx = next;
            } while (idx != -1);
        }

    public:
//        [[maybe_unused]] void HandleEventCallback(void* eventData) {
//            if (eventCallbacks.empty()) return;
//            for (auto iter = eventCallbacks.begin(); iter != eventCallbacks.end();) {
//                auto idx = *iter;
//                auto &coro = std::get<0>(nodes[idx].value);
//                auto &c = coro.Value();
//                assert(c.hasEventCallback && c.eventCallback);
//                auto r = c.eventCallback(eventData);
//                if (r < 0) {
//                    ++iter;
//                } else {
//                    WheelRemove(idx);
//                    Resume(idx, coro, c);
//                    if (r > 0) return;  // swallow
//                    iter = eventCallbacks.erase(iter);
//                }
//            }
//        }

        operator bool() const {
            return nodes.Count() > 0;
        }

        // c: Coro Func(...) { ... co_yield Cond().xxxx; ... co_return
        void Add(Generator<Cond> &&g) {
            if (g.Done()) return;
            auto &&c = g.Value();
            auto n = CalcSleepTimes(c);
            auto idx = nodes.Alloc(std::move(g), 0, -1, -1);
            auto wIdx = (cursor + n) % wheelLen;
            HandleCond(c, idx, wIdx);
        }

        // update time wheel, resume list coros
        void operator()() {
            HandleUpdateCallback();

            cursor = (cursor + 1) % ((int) wheelLen - 1);
            if (wheel[cursor] == -1) return;
            auto idx = wheel[cursor];
            assert(nodes[idx].prev == -1);
            wheel[cursor] = -1;
            do {
                auto &n = nodes[idx];
                assert(std::get<1>(n.value) == cursor);
                auto next = n.next;
                auto &coro = std::get<0>(n.value);
                auto &cond = coro.Value();
                if (cond.hasUpdateCallback) {
                    UpdateRemove(idx);
                }
//                if (cond.hasEventCallback) {
//                    EventRemove(idx);
//                }
                Resume(idx, coro, coro.Value());
                idx = next;
            } while (idx != -1);
        }
    };
}
