#pragma once

#include "xx_randoms.h"
#include "xx_lua_bind.h"

namespace xx::Lua {

    // id 映射
    template<> struct UserdataId<Random1> { static const uint8_t value = 1; };
    template<> struct UserdataId<Random2> { static const uint8_t value = 2; };
    template<> struct UserdataId<Random3> { static const uint8_t value = 3; };
    template<> struct UserdataId<Random4> { static const uint8_t value = 4; };

    // 值方式 meta 但是访问成员时转为指针
    template<typename T>
    struct MetaFuncs<T, std::enable_if_t<
               std::is_same_v<xx::Random1, std::decay_t<T>>
            || std::is_same_v<xx::Random2, std::decay_t<T>>
            || std::is_same_v<xx::Random3, std::decay_t<T>>
            || std::is_same_v<xx::Random4, std::decay_t<T>>
            >> {
        using U = std::decay_t<T>;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            SetType<U>(L);
            SetUserdataId<U>(L);
            SetFieldCClosure(L, "__tostring", [](auto L)->int { return Push(L, xx::ToString(*To<U*>(L))); });
            SetFieldCClosure(L, "NextInt", [](auto L)->int { return Push(L, To<U*>(L)->NextInt()); });
            SetFieldCClosure(L, "NextDouble", [](auto L)->int { return Push(L, To<U*>(L)->NextDouble()); });
            SetFieldCClosure(L, "Reset", [](auto L)->int {
                if (lua_gettop(L) == 1) {
                    To<U *>(L)->Reset();
                }
                else {
                    if constexpr (std::is_same_v<xx::Random1, std::decay_t<T>>) {
                        To<U *>(L)->Reset(To<int32_t>(L, 2));
                    }
                    if constexpr (std::is_same_v<xx::Random2, std::decay_t<T>>) {
                        To<U *>(L)->Reset(To<int64_t>(L, 2), To<int64_t>(L, 3));
                    }
                    if constexpr (std::is_same_v<xx::Random3, std::decay_t<T>>) {
                        To<U *>(L)->Reset(To<int64_t>(L, 2));
                    }
                    if constexpr (std::is_same_v<xx::Random4, std::decay_t<T>>) {
                        To<U *>(L)->Reset(To<int32_t>(L, 2), To<int64_t>(L, 3));
                    }
                }
                return 0;
            });
        }
    };
    // 值方式 push
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<
            std::is_same_v<xx::Random1, std::decay_t<T>>
            || std::is_same_v<xx::Random2, std::decay_t<T>>
            || std::is_same_v<xx::Random3, std::decay_t<T>>
            || std::is_same_v<xx::Random4, std::decay_t<T>>
            >> {
        static constexpr int checkStackSize = 1;
        static int Push_(lua_State* const& L, T&& in) {
            return PushUserdata<std::decay_t<T>>(L, std::forward<T>(in));
        }
    };
    // 指针方式 to 但是做 值方式 检查
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>
            &&(std::is_same_v<xx::Random1, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>
            || std::is_same_v<xx::Random2, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>
            || std::is_same_v<xx::Random3, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>
            || std::is_same_v<xx::Random4, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>)
            >> {
        static void To_(lua_State* const& L, int const& idx, T& out) {
            AssertType<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>(L, idx);
            out = (T)lua_touserdata(L, idx);
        }
    };

    // 启动时自动注册
    struct RandomsAutoRegisterFunc {
        RandomsAutoRegisterFunc() {
            uwtfs[UserdataId_v<Random1>] = [](lua_State* const& L, xx::Data& d, int const& idx) {
                d.Write(*To<Random1*>(L, idx));
            };
            uwtfs[UserdataId_v<Random2>] = [](lua_State* const& L, xx::Data& d, int const& idx) {
                d.Write(*To<Random2*>(L, idx));
            };
            uwtfs[UserdataId_v<Random3>] = [](lua_State* const& L, xx::Data& d, int const& idx) {
                d.Write(*To<Random3*>(L, idx));
            };
            uwtfs[UserdataId_v<Random4>] = [](lua_State* const& L, xx::Data& d, int const& idx) {
                d.Write(*To<Random4*>(L, idx));
            };

            urffs[UserdataId_v<Random1>] = [](lua_State* const& L, xx::Data_r& d)->int {
                int seed;
                if (int r = d.ReadFixed(seed)) return r;
                PushUserdata<Random1>(L, seed);
                return 0;
            };
            urffs[UserdataId_v<Random2>] = [](lua_State* const& L, xx::Data_r& d)->int {
                uint64_t x, w;
                if (int r = d.ReadFixed(x)) return r;
                if (int r = d.ReadFixed(w)) return r;
                PushUserdata<Random2>(L, x, w);
                return 0;
            };
            urffs[UserdataId_v<Random3>] = [](lua_State* const& L, xx::Data_r& d)->int {
                uint64_t seed;
                if (int r = d.ReadFixed(seed)) return r;
                PushUserdata<Random3>(L, seed);
                return 0;
            };
            urffs[UserdataId_v<Random4>] = [](lua_State* const& L, xx::Data_r& d)->int {
                typename Random4::SeedType seed;
                uint64_t count;
                if (int r = d.ReadFixed(seed)) return r;
                if (int r = d.Read(count)) return r;
                PushUserdata<Random4>(L, seed, count);
                return 0;
            };
        }
    };
    inline static RandomsAutoRegisterFunc __rafcf;
}

namespace xx::Lua::Randoms {
    // 在 lua 中注册 全局的 Data 创建函数
    inline void Register(lua_State *const &L) {
        SetGlobalCClosure(L, "NewXxRandom1", [](auto L) -> int { return PushUserdata<xx::Random1>(L); });
        SetGlobalCClosure(L, "NewXxRandom2", [](auto L) -> int { return PushUserdata<xx::Random2>(L); });
        SetGlobalCClosure(L, "NewXxRandom3", [](auto L) -> int { return PushUserdata<xx::Random3>(L); });
        SetGlobalCClosure(L, "NewXxRandom4", [](auto L) -> int { return PushUserdata<xx::Random4>(L); });
    }
}
