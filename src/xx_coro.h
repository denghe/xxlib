#pragma once

#include <exception>
#include <tuple>
#include <functional>
#include <unordered_set>
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
                uint8_t hasEventCallback;
                // more
            };
        };

        int sleepTimes = 0;
        double sleepSeconds = 0.0;
        std::function<bool()> updateCallback;
        std::function<bool()> eventCallback;
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
        Cond &&UpdateCallback(F&& cbFunc) {
            hasUpdateCallback = true;
            updateCallback = std::forward<F>(cbFunc);
            return std::move(*this);
        }

        template<typename F, class ENABLED = std::enable_if_t<!std::is_arithmetic_v<F>>>
        Cond &&EventCallback(F&& cbFunc) {
            hasEventCallback = true;
            eventCallback = std::forward<F>(cbFunc);
            return std::move(*this);
        }

        // more
    };

    using Coro = Generator<Cond>;

    struct Coros {
        Coros(Coros const &) = delete;

        Coros &operator=(Coros const &) = delete;

        Coros(Coros &&) = default;

        Coros &operator=(Coros &&) = default;

        // 0: wheel index
        NodePool<std::tuple<int, Generator<Cond>>> nodes;

        int wheelLen;
        int *wheel;
        int cursor = 0;
        double frameDelaySeconds;

        std::unordered_set<int> updateCallbacks;
        std::unordered_set<int> eventCallbacks;

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
            updateCallbacks.clear();
            eventCallbacks.clear();
        }

    protected:

        void WheelAdd(int const &idx, int const &wIdx) {
            auto& n = nodes[idx];
            n.prev = -1;
            n.next = wheel[wIdx];
            std::get<0>(n.value) = wIdx;
            if (wheel[wIdx] >= 0) {
                nodes[wheel[wIdx]].prev = idx;
            }
            wheel[wIdx] = idx;
        }

        void WheelRemove(int const &idx) {
            auto& n = nodes[idx];
            assert(n.prev != -2);
            auto& wIdx = std::get<0>(n.value);
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

        int CalcSleepTimes(Cond const &c) const {
            int n;
            if (c.hasSleepTimes) {
                assert(c.sleepTimes > 0);
                n = c.sleepTimes;
            } else if (c.hasSleepSeconds) {
                assert(c.sleepSeconds > 0);
                n = (int) (c.sleepSeconds / frameDelaySeconds);
                if (n == 0) {
                    n == 1;
                }
            } else {
                n = 1;
            }
            assert(n < wheelLen);
            return n;
        }

        void HandleCond(Cond& cond, int const& idx, int const& wIdx) {
            WheelAdd(idx, wIdx);
            if (cond.hasUpdateCallback) {
                assert(!updateCallbacks.contains(idx));
                updateCallbacks.insert(idx);
            }
            if (cond.hasEventCallback) {
                assert(!eventCallbacks.contains(idx));
                eventCallbacks.insert(idx);
            }
            // todo: more
        }

        void Resume(int const& idx, Coro& coro, Cond& c) {
            if (coro.Resume()) {
                if (c.hasUpdateCallback) {
                    assert(updateCallbacks.contains(idx));
                    updateCallbacks.erase(idx);
                }
                if (c.hasEventCallback) {
                    assert(eventCallbacks.contains(idx));
                    eventCallbacks.erase(idx);
                }
                nodes.Free(idx);
                assert(nodes.Count() >= updateCallbacks.size());
                assert(nodes.Count() >= eventCallbacks.size());
            } else {
                auto num = CalcSleepTimes(c);
                auto wIdx = (cursor + num) % wheelLen;
                HandleCond(c, idx, wIdx);
            }
        }

        void HandleUpdateCallback() {
            if (updateCallbacks.empty()) return;
            for (auto iter = updateCallbacks.begin(); iter != updateCallbacks.end(); ) {
                auto idx = *iter;
                auto& coro = std::get<1>(nodes[idx].value);
                auto& c = coro.Value();
                assert(c.hasUpdateCallback && c.updateCallback);
                auto r = c.updateCallback();
                if (r) {
                    WheelRemove(idx);
                    Resume(idx, coro, c);
                    iter = updateCallbacks.erase(iter);
                }
                else ++iter;
            }
        }

    public:
        void HandleEventCallback() {
            if (eventCallbacks.empty()) return;
            for (auto iter = eventCallbacks.begin(); iter != eventCallbacks.end(); ) {
                auto idx = *iter;
                auto& coro = std::get<1>(nodes[idx].value);
                auto& c = coro.Value();
                assert(c.hasEventCallback && c.eventCallback);
                auto r = c.eventCallback();
                if (r) {
                    WheelRemove(idx);
                    Resume(idx, coro, c);
                    iter = eventCallbacks.erase(iter);
                }
                else ++iter;
            }
        }

        operator bool() const {
            return nodes.Count() > 0;
        }

        // c: Coro Func(...) { ... co_yield Cond().xxxx; ... co_return
        void Add(Generator<Cond> &&g) {
            if (g.Done()) return;
            auto&& c = g.Value();
            auto n = CalcSleepTimes(c);
            auto idx = nodes.Alloc(0, std::move(g));
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
                auto& n = nodes[idx];
                auto next = n.next;
                auto& coro = std::get<1>(n.value);
                Resume(idx, coro, coro.Value());
                idx = next;
            } while (idx != -1);
        }
    };
}
