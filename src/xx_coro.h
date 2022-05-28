#pragma once

#include <exception>
#include <tuple>
#include <variant>
#include <functional>
#include <unordered_map>
#include <memory>
#include "xx_nodepool.h"

#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#else
static_assert(false, "No co_await support");
#endif

#define CoAwait(func) {auto&& g = func; while(!g.Done()) { co_yield g.Value(); g.Resume(); }}

namespace cxx14 {}
namespace xx {
    using namespace std;
#if __has_include(<coroutine>)
#else
    using namespace std::experimental;
    using namespace cxx14;
#endif

    // todo
    // 已知问题: 如果用 lambda 带捕获列表 返回 Generator<> 就会出问题。捕获列表中的数据似乎被破坏了
    // 所以当前最好传参绕过捕获需求

    template<typename T>
    struct Generator {
        struct promise_type {
            variant<monostate, T, exception_ptr> r;
            suspend_never initial_suspend() { return {};}
            suspend_always final_suspend() noexcept(true) { return {}; }
            auto get_return_object() { return Generator{*this}; }
            void return_void() {}
            suspend_always yield_value(T v) { r.template emplace<1>(std::move(v)); return {}; }
            void unhandled_exception() { r.template emplace<2>(std::current_exception()); }
        };

        Generator(Generator const& o) = delete;
        Generator(Generator &&o) noexcept : h(o.h) { o.h = nullptr; };
        ~Generator() { if (h) { h.destroy(); } }
        explicit Generator(promise_type& p) : h(coroutine_handle<promise_type>::from_promise(p)) {}
        Generator& operator=(Generator const&) = delete;
        Generator& operator=(Generator &&) = delete;

        void Resume() { assert(h); h.resume(); }
        bool Done() { assert(h); return h.done(); }
        T &Value() {
            assert(h);
            auto& r = h.promise().r;
            if (r.index() == 1) return get<1>(r);
            std::rethrow_exception(get<2>(r));
        }

    private:
        coroutine_handle<promise_type> h;
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

        Cond(int const &sleepSeconds) : sleepSeconds((float)sleepSeconds) {}

        Cond(float const &sleepSeconds) : sleepSeconds(sleepSeconds) {}

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
        NodePool<std::pair<Generator<Cond>, Cond*>, 16, true, true> nodes;

        int wheelLen;
        int *wheel;
        int cursor = 0;
        float frameDelaySeconds;

        int updateList = -1;

        // key: eventKey   value: node index
        std::unordered_map<int, int> eventKeyMappings;

        // last coro resume state
        bool isTimeout = false;

        explicit Coros(float const &framePerSeconds = 10, int const &wheelLen = 10 * 60 * 5)
                : frameDelaySeconds(1.0f / framePerSeconds), wheelLen(wheelLen) {
            wheel = (int *) malloc(wheelLen * sizeof(int));
            memset(wheel, -1, wheelLen * sizeof(int));
        }

        ~Coros() {
            nodes.Clear();
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
            assert(eventKeyMappings.find(cond.eventKey) == eventKeyMappings.end());
            auto r = eventKeyMappings.emplace(cond.eventKey, idx);
            assert(r.second);
        }

        void EventRemove(Cond &cond, int const &idx) {
            assert(eventKeyMappings.find(cond.eventKey) != eventKeyMappings.end());
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
            assert(!coro.Done());
            coro.Resume();
            if (coro.Done()) {
                nodes.Free(idx);
            } else {
                assert(std::addressof(c) == std::addressof(coro.Value()));
                auto num = CalcSleepTimes(c);
                auto wIdx = (cursor + num) % wheelLen;
                HandleCond(c, idx, wIdx);
            }
        }

    public:
        [[maybe_unused]] void FireEvent(int const& eventKey) {
            auto iter = eventKeyMappings.find(eventKey);
            if (iter != eventKeyMappings.end()) {
                isTimeout = false;
                auto idx = iter->second;
                auto &coro = nodes[idx].value.first;
                auto &c = *nodes[idx].value.second;
                assert(c.hasEvent);
                EventRemove(c, idx);
                if (c.hasUpdate) {
                    UpdateRemove(c, idx);
                }
                WheelRemove(c, idx);
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
            if (updateList != -1) {
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
                        isTimeout = false;
                        Resume(idx, coro, c);
                    }
                    idx = next;
                } while (idx != -1);
            }

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
                isTimeout = true;
                Resume(idx, coro, c);
                idx = next;
            } while (idx != -1);
        }
    };
}

/*
    // coroutine support
    xx::Coros coros;

    template<typename Rtv>
    struct Task {
        Server* server;
        xx::Weak<Peer> wp;
        Rtv r;
        int ek;
        explicit Task(Peer* const& p) : server((Server*)p->ec), wp(xx::SharedFromThis(p)) {
            ek = p->GenSerial();
        }
        void Dispatch() {
            server->Dispatch([this] {
                if (auto p = wp.Lock()) {
                    p->coros.FireEvent(ek);
                }
            });
        }
    };

    template<typename Rtv, typename Func>
    auto NewTask(Func &&func) {
        auto rtv = std::make_shared<Task<Rtv>>(this);
        ((Server *) ec)->db->tp.Add([w = std::weak_ptr<Task<Rtv>>(rtv), func = std::forward<Func>(func)](DB::Env &env) {
            auto r = func(env);
            if (auto s = w.lock()) {
                s->r = std::move(r);
                s->Dispatch();
            }
        });
        return rtv;
    }

    template<typename PKG = xx::ObjBase, typename Sender, typename Rtv, typename ... Args>
    int CoroSendRequest(Sender& sender, Rtv &rtv, Args const &... args) {
        assert(sender && sender->Alive());
        return sender->template SendRequest<PKG>([w = xx::SharedFromThis(this).ToWeak(), &rtv](int32_t const &eventKey, xx::ObjBase_s &&ob) {
            if (auto vp = w.Lock()) {
                rtv = std::move(ob);
                vp->coros.FireEvent(eventKey);
            }
        }, 99.0, args...);
    }
*/
