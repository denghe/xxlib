#pragma once

#include <vector>

#if defined(__clang__)
#include <experimental/coroutine>
#else

#include <coroutine>
#include <exception>

#endif

#define CoAwait(func) {auto&& g = func; while(!g.Resume()) { co_yield g.Value(); }}

namespace xx {
#if defined(__clang__)
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
        Generator(Generator &&o) {
            h = o.h;
            o.h = {};
        };

        // can't use == default here
        Generator &operator=(Generator &&o) {
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

            auto initial_suspend() {
                return suspend_always{};
            }

            auto final_suspend() noexcept {
                return suspend_always{};
            }

            auto get_return_object() {
                return Generator{Handle::from_promise(*this)};
            }

            void return_void() {}

            template<std::convertible_to<T> From>
            auto yield_value(From &&some_value) {
                v = std::forward<From>(some_value);
                return suspend_always{};
            }

            void unhandled_exception() {
                e = std::current_exception();
            }
        };
    };
}
