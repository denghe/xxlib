#pragma once

// important: only support static function or lambda !!!  COPY data from arguments !!! do not ref !!!

#include "xx_coro_simple.h"
#include "xx_task.h"

#include "xx_typetraits.h"
#include "xx_time.h"
#include "xx_list.h"
#include "xx_listlink.h"

namespace xx {

    template<typename CoroType, typename WeakType>
    struct CorosBase {
        xx::ListLink<std::pair<WeakType, CoroType>, int32_t> coros;
    };

    template<typename CoroType>
    struct CorosBase<CoroType, void> {
        xx::ListLink<CoroType, int32_t> coros;
    };

    template<typename CoroType, typename WeakType>
    struct Coros_ : CorosBase<CoroType, WeakType> {
        Coros_(Coros_ const&) = delete;
        Coros_& operator=(Coros_ const&) = delete;
        Coros_(Coros_&&) noexcept = default;
        Coros_& operator=(Coros_&&) noexcept = default;
        explicit Coros_(int32_t cap = 8) {
            this->coros.Reserve(cap);
        }

        template<typename WT, typename CT>
        void Add(WT&& w, CT&& c) {
            if (!w || c) return;
            this->coros.Emplace(std::pair<WeakType, CoroType> { std::forward<WT>(w), std::forward<CT>(c) });
        }

        template<typename CT>
        void Add(CT&& c) {
            if (c) return;
            this->coros.Emplace(std::forward<CT>(c));
        }

        template<typename F>
        int AddLambda(F&& f) {
            return Add([](F f)->CoroType {
                co_await f();
            }(std::forward<F>(f)));
        }

        void Clear() {
            this->coros.Clear();
        }

        int32_t operator()() {
            int prev = -1, next{};
            for (auto idx = this->coros.head; idx != -1;) {
                auto& o = this->coros[idx];
                bool needRemove;
                if constexpr(std::is_void_v<WeakType>) {
                    needRemove = o.Resume();
                } else {
                    needRemove = !o.first || o.second.Resume();
                }
                if (needRemove) {
                    next = this->coros.Remove(idx, prev);
                } else {
                    next = this->coros.Next(idx);
                    prev = idx;
                }
                idx = next;
            }
            return this->coros.Count();
        }

        [[nodiscard]] int32_t Count() const {
            return this->coros.Count();
        }

        [[nodiscard]] bool Empty() const {
            return !this->coros.Count();
        }

        void Reserve(int32_t cap) {
            this->coros.Reserve(cap);
        }
    };

    using Coros = Coros_<Coro, void>;
    template<typename WeakType>
    using CondCoros = Coros_<Coro, WeakType>;  // check condition before Resume ( for life cycle manage )

    using Tasks = Coros_<Task<>, void>;
    template<typename WeakType>
    using CondTasks = Coros_<Task<>, WeakType>;


    /***********************************************************************************************************/
    /***********************************************************************************************************/

    template<typename KeyType, typename DataType>
    using EventArgs = std::pair<KeyType, DataType&>;

    template<typename KeyType, typename DataType>
    using EventYieldType = std::variant<int, EventArgs<KeyType, DataType>>;   // int for yield once

    template<bool trueCoro_falseTask, typename KeyType, typename DataType>
    using EventCoro = std::conditional_t<trueCoro_falseTask
            , Coro_<EventYieldType<KeyType, DataType>>
            , Task<void, EventYieldType<KeyType, DataType>>>;

    template<bool trueCoro_falseTask, typename KeyType, typename DataType, int timeoutSecs = 15>
    struct EventCoros {
        using CoroType = EventCoro<trueCoro_falseTask, KeyType, DataType>;
        using Args = EventArgs<KeyType, DataType>;
        using Tuple = std::tuple<Args, double, CoroType>;
        xx::List<Tuple, int> condCoros;
        xx::List<CoroType, int> updateCoros;

        template<std::convertible_to<CoroType> CT>
        int Add(CT&& c) {
            if (c) return 0;
            auto& y = c.coro.promise().y;
            if (y.index() == 0) {
                updateCoros.Emplace(std::forward<CT>(c));
            } else if (y.index() == 1) {
                condCoros.Emplace(std::move(std::get<Args>(y)), xx::NowSteadyEpochSeconds() + timeoutSecs, std::forward<CT>(c) );
            } else if (y.index() == 2) return std::get<int>(y);
            return 0;
        }

        int Add(Task<>&& c) {
            return Add([](Task<> c)->CoroType {
                co_await c;
            }(std::move(c)));
        }

        template<typename F>
        int AddLambda(F&& f) {
            return Add([](F f)->CoroType {
                co_await f();
            }(std::forward<F>(f)));
        }

        // match key & store d & resume coro
        // null: dismatch     0: success      !0: error ( need close ? )
        template<typename DT = DataType>
        std::optional<int> Trigger(KeyType const& v, DT&& d) {
            if (condCoros.Empty()) return false;
            for (int i = condCoros.len - 1; i >= 0; --i) {
                auto& tup = condCoros[i];
                if (v == std::get<Args>(tup).first) {
                    std::get<Args>(tup).second = std::forward<DT>(d);
                    if (int r = Resume(i)) return r;
                    return 0;
                }
            }
            return {};
        }

        // handle condCoros timeout & resume updateCoros
        decltype(auto) operator()() {
            if (!condCoros.Empty()) {
                auto now = xx::NowSteadyEpochSeconds();
                for (int i = condCoros.len - 1; i >= 0; --i) {
                    auto& tup = condCoros[i];
                    if (std::get<double>(tup) < now) {
                        std::get<Args>(tup).second = {};
                        if (int r = Resume(i)) return r;
                    }
                }
            }
            if (!updateCoros.Empty()) {
                for (int i = updateCoros.len - 1; i >= 0; --i) {
                    if (auto& c = updateCoros[i]; c.Resume()) {
                        updateCoros.SwapRemoveAt(i);
                    } else {
                        auto& y = c.coro.promise().y;
                        if (y.index() != 0) {
                            condCoros.Emplace(Tuple{ std::move(std::get<Args>(y)), xx::NowSteadyEpochSeconds() + timeoutSecs, std::move(c) });
                            updateCoros.SwapRemoveAt(i);
                        } // else y other logic?
                    }
                }
            }
            return condCoros.len + updateCoros.len;
        }

    protected:
        XX_FORCE_INLINE int Resume(int i) {
            auto& tup = condCoros[i];
            auto& args = std::get<Args>(tup);
            auto& c = std::get<CoroType>(tup);
            if (c.Resume()) {
                condCoros.SwapRemoveAt(i);
            } else {
                auto& y = c.coro.promise().y;
                if (y.index() == 0) {
                    updateCoros.Emplace(std::move(c));
                    condCoros.SwapRemoveAt(i);
                } else if (y.index() == 1) {
                    args = std::move(std::get<Args>(y));
                    std::get<double>(tup) = xx::NowSteadyEpochSeconds() + timeoutSecs;
                } else return std::get<int>(y);
            }
            return 0;
        }
    };

    template<typename KeyType, typename DataType, int timeoutSecs = 15>
    using EventTasks = EventCoros<false, KeyType, DataType, timeoutSecs>;

}
