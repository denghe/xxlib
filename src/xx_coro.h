#pragma once

#include <exception>
#include <tuple>
#include <functional>
#include <unordered_map>
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

        uint8_t hasEvent = 0;
        uint8_t hasUpdate = 0;

        float sleepSeconds = 0.0;
        int wIdx = -1;

        int eventKey = 0;

        int updatePrev = -1, updateNext = -1;
        std::function<bool()> updateFunc;


        Cond() = default;

        Cond(float const &sleepSeconds) : sleepSeconds(sleepSeconds) {
        }

        [[maybe_unused]] Cond &&Event(int const& eventKey_) {
            assert(!hasEvent);
            hasEvent = true;
            eventKey = eventKey_;
            return std::move(*this);
        }

        template<typename F>
        Cond &&Update(F &&cbFunc) {
            assert(!hasUpdate);
            hasUpdate = true;
            updateFunc = std::forward<F>(cbFunc);
            return std::move(*this);
        }

    protected:
        template<std::size_t...Idxs, typename T>
        static int Wait_(std::index_sequence<Idxs...>, T const &t) {
            return (... + (**std::get<Idxs>(t) ? 1 : 0));
        }

    public:
        // flag: std::shared_ptr<bool>
        // Update for wait all tasks complete. n = completed tasks count
        template<typename...Flags>
        [[maybe_unused]] Cond &&Wait(int& n, Flags &...flags) {
            static_assert(sizeof...(Flags) > 0);
            assert(!hasUpdate);
            return Update([&n, t = std::make_tuple(&flags...)] {
                n = Wait_(std::make_index_sequence<sizeof...(Flags)>(), t);
                return n == sizeof...(Flags);
            });
        }
    };

    using Coro = Generator<Cond>;

    struct Coros {
        Coros(Coros const &) = delete;

        Coros &operator=(Coros const &) = delete;

        Coros(Coros &&) = default;

        Coros &operator=(Coros &&) = default;

        // 1: wheel index
        NodePool<std::pair<Generator<Cond>, Cond*>> nodes;

        int wheelLen;
        int *wheel;
        int cursor = 0;
        float frameDelaySeconds;

        int updateList = -1;

        // key: eventKey   value: node index
        std::unordered_map<int, int> eventKeyMappings;

        explicit Coros(float const &framePerSeconds = 10, int const &wheelLen = 10 * 60 * 5)
                : frameDelaySeconds(1.0f / framePerSeconds), wheelLen(wheelLen) {
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
            eventKeyMappings.clear();
        }

    protected:

        void WheelAdd(Cond &c, int const &idx, int const &wIdx) {
            auto &n = nodes[idx];
            n.prev = -1;
            n.next = wheel[wIdx];
            c.wIdx = wIdx;
            if (wheel[wIdx] >= 0) {
                nodes[wheel[wIdx]].prev = idx;
            }
            wheel[wIdx] = idx;
        }

        void WheelRemove(Cond &c, int const &idx) {
            auto &n = nodes[idx];
            assert(n.prev != -2);
            assert(c.wIdx >= 0);
            if (n.prev < 0 && wheel[c.wIdx] == idx) {
                wheel[c.wIdx] = n.next;
            } else {
                nodes[n.prev].next = n.next;
            }
            if (n.next >= 0) {
                nodes[n.next].prev = n.prev;
            }
            c.wIdx = -1;
        }

        Cond &GetCond(int const& idx) {
            return *nodes[idx].value.second;
        }

        void UpdateAdd(Cond &c, int const &idx) {
            c.updatePrev = -1;
            c.updateNext = updateList;
            if (updateList != -1) {
                GetCond(updateList).updatePrev = idx;
            }
            updateList = idx;
            //std::cout << __LINE__ << " updateList = " << updateList << std::endl;
        }

        void UpdateRemove(Cond& cond, int const &idx) {
            if (updateList == idx) {
                assert(cond.updatePrev == -1);
                updateList = cond.updateNext;
                //std::cout << __LINE__ << " updateList = " << updateList << std::endl;
            } else {
                assert(cond.updatePrev != -1);
                GetCond(cond.updatePrev).updateNext = cond.updateNext;
            }
            if (cond.updateNext != -1) {
                assert(GetCond(cond.updateNext).updatePrev == idx);
                GetCond(cond.updateNext).updatePrev = cond.updatePrev;
            }
        }

        void EventAdd(Cond &cond, int const &idx) {
            assert(eventKeyMappings.contains(cond.eventKey));
            auto r = eventKeyMappings.emplace(cond.eventKey, idx);
            assert(r.second);
        }

        void EventRemove(Cond &cond, int const &idx) {
            assert(eventKeyMappings.contains(cond.eventKey));
            assert(eventKeyMappings[cond.eventKey] == idx);
            eventKeyMappings.erase(cond.eventKey);
        }

        int CalcSleepTimes(Cond const &c) const {
            auto n = (int) (c.sleepSeconds / frameDelaySeconds);
            assert(n < wheelLen);
            if (n == 0) {
                n = 1;
            }
            return n;
        }

        // for Add / Resume
        void HandleCond(Cond &c, int const &idx, int const &wIdx) {
            WheelAdd(c, idx, wIdx);
            if (c.hasUpdate) {
                UpdateAdd(c, idx);
            }
            if (c.hasEvent) {
                EventAdd(c, idx);
            }
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

        void HandleUpdate() {
            if (updateList == -1) return;
            auto idx = updateList;
            do {
                auto &coro = nodes[idx].value.first;
                auto &c = *nodes[idx].value.second;
                auto next = c.updateNext;
                assert(c.hasUpdate);
                assert(c.updateFunc);
                auto r = c.updateFunc();
                //std::cout << __LINE__ << " r = " << r << std::endl;
                if (r) {
                    UpdateRemove(c, idx);
                    WheelRemove(c, idx);
                    Resume(idx, coro, c);
                }
                idx = next;
            } while (idx != -1);
        }

    public:
        [[maybe_unused]] void FireEvent(int const& eventKey) {
            auto iter = eventKeyMappings.find(eventKey);
            if (iter != eventKeyMappings.end()) {
                auto idx = iter->second;
                auto &coro = nodes[idx].value.first;
                auto &c = *nodes[idx].value.second;
                Resume(idx, coro, c);
            }
        }

        operator bool() const {
            return nodes.Count() > 0;
        }

        // c: Coro Func(...) { ... co_yield Cond().xxxx; ... co_return
        void Add(Generator<Cond> &&g) {
            if (g.Done()) return;
            auto &&c = g.Value();
            auto n = CalcSleepTimes(c);
            auto idx = nodes.Alloc(std::move(g), &c);
            auto wIdx = (cursor + n) % wheelLen;
            HandleCond(c, idx, wIdx);
        }

        // update time wheel, resume list coros
        void operator()() {
            HandleUpdate();

            cursor = (cursor + 1) % ((int) wheelLen - 1);
            if (wheel[cursor] == -1) return;
            auto idx = wheel[cursor];
            assert(nodes[idx].prev == -1);
            wheel[cursor] = -1;
            do {
                auto &n = nodes[idx];
                auto next = n.next;
                auto &coro = n.value.first;
                auto &c = *n.value.second;
                assert(c.wIdx == cursor);
                if (c.hasUpdate) {
                    UpdateRemove(c, idx);
                }
                if (c.hasEvent) {
                    EventRemove(c, idx);
                }
                Resume(idx, coro, c);
                idx = next;
            } while (idx != -1);
        }
    };

    // Coros ext for Peer
    template<typename T>
    struct CorosExt {
        // coroutine support
        xx::Coros coros;

        /* example:

std::optional<DB::Rtv<DB::AccountInfo>> rtv;
co_yield xx::Cond(15).Event(NewTask(rtv, [o = std::move(o)](DB::Env &db) mutable {
    return db.TryGetAccountInfoByUsernamePassword(o->username, o->password);
}));

if (rtv.has_value()) {} else {}
        */
        template<typename ThreadPool, typename Dispatcher, typename Rtv, typename Func>
        int NewCoroTask(ThreadPool& tp, Dispatcher& d, Rtv& rtv, Func&& func) {
            auto serial = ((T*)this)->GenSerial();
            tp.Add([&d, w = xx::SharedFromThis(this).ToWeak(), serial, &rtv, func = std::forward<Func>(func)](DB::Env &env) mutable {
                d.Dispatch([ w = std::move(w), serial, &rtv, result = func(env) ]() mutable {
                    if (auto p = w.Lock()) {
                        rtv = std::move(result);
                        p->coros.FireEvent(serial);
                    }
                });
            });
            return serial;
        }

        template<typename Rtv,typename ... Args>
        int SendRequest(Rtv& rtv, Args const&... args) {
            return SendRequest([this, &rtv](auto serial, Rtv&& ob) {
                rtv = std::move(ob);
                FireEvent(serial);
            }, 99999, std::forward<Args>(args)...);
        }

//        template<typename Rtv, typename Func>
//        int NewTask(Rtv& rtv, Func&& func) {
//            return NewCoroTask(((Server*)(ec))->db->tp, *(Server*)(ec), rtv, std::forward<Func>(func));
//        }

    };
}

