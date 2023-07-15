#pragma once
// important: only support static function or lambda !!!  COPY data from arguments !!! do not ref !!!

#include "xx_typetraits.h"

namespace xx {
    template<typename R = void, typename Y = int>
    struct Task;

    namespace detail {
        template<typename Derived, typename R, typename Y>
        struct PromiseBase {
            std::coroutine_handle<> prev, last;
            PromiseBase *root{ this };
            Y y;

            struct FinalAwaiter {
                bool await_ready() const noexcept { return false; }
                void await_resume() noexcept {}
                template<typename promise_type>
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> curr) noexcept {
                    if (auto &p = curr.promise(); p.prev) return p.prev;
                    else return std::noop_coroutine();
                }
            };
            Task<R, Y> get_return_object() noexcept {
                auto tmp = Task<R, Y>{ std::coroutine_handle<Derived>::from_promise(*(Derived*)this) };
                tmp.coro.promise().last = tmp.coro;
                return tmp;
            }
            std::suspend_always initial_suspend() { return {}; }
            FinalAwaiter final_suspend() noexcept(true) { return {}; }
            template<std::convertible_to<Y> U>
            std::suspend_always yield_value(U&& v) {
                root->y = std::forward<U>(v);
                return {};
            }
            void unhandled_exception() { throw; }
        };

        template<typename R, typename Y>
        struct Promise final : PromiseBase<Promise<R, Y>, R, Y> {
            std::optional<R> r;

            template<typename T>
            void return_value(T&& v) { r.emplace(std::forward<T>(v)); }
        };

        template<typename Y>
        struct Promise<void, Y> : PromiseBase<Promise<void, Y>, void, Y> {
            void return_void() noexcept {}
        };
    }

    template<typename R, typename Y>
    struct [[nodiscard]] Task {
        using promise_type = detail::Promise<R, Y>;
        using H = std::coroutine_handle<promise_type>;
        H coro;

        Task() = delete;
        Task(H h) { coro = h; }
        Task(Task const &o) = delete;
        Task &operator=(Task const &o) = delete;
        Task(Task &&o) : coro(std::exchange(o.coro, nullptr)) {}
        Task &operator=(Task &&o) {
            std::swap(coro, o.coro);
            return *this;
        }
        ~Task() { if (coro) { coro.destroy(); } }

        struct Awaiter {
            bool await_ready() const noexcept { return false; }
            decltype(auto) await_suspend(std::coroutine_handle<> prev) noexcept {
                auto& cp = curr.promise();
                cp.prev = prev;
                cp.root = ((H&)prev).promise().root;
                cp.root->last = curr;
                return curr;
            }
            decltype(auto) await_resume() {
                assert(curr.done());
                auto& p = curr.promise();
                p.root->last = p.prev;
                if constexpr (std::is_same_v<void, R>) return;
                else return *p.r;
            }
            H curr;
        };
        Awaiter operator co_await() const& noexcept { return {coro}; }

        decltype(auto) Result() const { return *coro.promise().r; }
        decltype(auto) YieldValue() const { return coro.promise().y; }

        template<bool runOnce = false>
        XX_FORCE_INLINE decltype(auto) Run() {
            auto& p = coro.promise();
            auto& c = p.last;
            while(c && !c.done()) {
                c.resume();
                if constexpr(runOnce) return;
            }
            if constexpr (!std::is_void_v<R>) return *p.r;
        }
        operator bool() const { return /*!coro ||*/ coro.done(); }
        void operator()() { Run<true>(); }
        bool Resume() { Run<true>(); return *this; }
    };

    template<typename R, typename Y>
    struct IsPod<Task<R, Y>> : std::true_type {};
}

