#pragma once
#include <vector>
#if defined(__clang__)
#include <experimental/coroutine>
#else
#include <coroutine>
#include <exception>

#endif
#define CoYield co_yield 0  // gcc 下比 co_await std::suspend_always{} 快
#define CoAwait(func) {auto&& g = func; while(g.Next()) {CoYield;}}
#define CoAsync xx::Generator<int>
#define CoRtv CoAsync
#define CoReturn co_return

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
        Generator(Generator &&o) = default;
        Generator &operator=(Generator &&o) = default;
        ~Generator() {
            if (h) {
                h.destroy();
            }
        }
        bool Next() {
            h.resume();
            return !h.done();
        }
        [[nodiscard]] T const& Value() const {
            return h.promise().v;
        }
        T & Value() {
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
            auto yield_value(From&& some_value) {
                v = std::forward<From>(some_value);
                return suspend_always{};
            }

            void unhandled_exception() {
                e = std::current_exception();
            }
        };
    };

    struct Coros {
        std::vector<CoRtv > cs;
        inline void Add(CoRtv &&c) {
            cs.emplace_back(std::move(c));
        }
        inline bool Update() {
            // 倒扫以方便交换删除
            for (auto &&i = cs.size() - 1; i != (size_t) -1; --i) {
                if (!cs[i].Next()) {
                    if (i < cs.size() - 1) {
                        std::swap(cs[cs.size() - 1], cs[i]);
                    }
                    cs.pop_back();
                }
            }
            return cs.empty();
        }
    };
}


//
//    struct iterator_end {
//    };
//
//    struct iterator {
//        template<class>
//        friend
//        class Generator;
//
//        using iterator_category = std::input_iterator_tag;
//        using value_type = T;
//
//        T const &operator*() const {
//            return p->v;
//        }
//
//        T &operator*() {
//            return p->v;
//        }
//
//        void operator++() {
//            Handle::from_promise(*p).resume();
//        }
//
//        bool operator!=(iterator_end) {
//            return !Handle::from_promise(*p).done();
//        }
//
//    private:
//        explicit iterator(promise_type *const &p) : p(p) {}
//
//        promise_type *p;
//    };
//
//    iterator begin() { return iterator(&h.promise()); }
//
//    iterator_end end() { return {}; }
