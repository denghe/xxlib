#pragma once

#include "xx_lua_bind.h"
#include "xx_randoms.h"

namespace xx::Lua {

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
            SetField(L, "typeId", U::typeId);   // for easy WriteTo & ReadFrom data
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
                return 0; });
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
        static int Push(lua_State* const& L, T&& in) {
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
        static void To(lua_State* const& L, int const& idx, T& out) {
            AssertType<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>(L, idx);
            out = (T)lua_touserdata(L, idx);
        }
    };

    // 写 id 映射
    template<> struct UserdataId<Random1> { static const uint16_t value = 1; };
    template<> struct UserdataId<Random2> { static const uint16_t value = 2; };
    template<> struct UserdataId<Random3> { static const uint16_t value = 3; };
    template<> struct UserdataId<Random4> { static const uint16_t value = 4; };
}

namespace xx::Lua::Randoms {

    // 从 Data
    inline void ReadFrom(lua_State* const& L, xx::Data_r& d) {
        // todo
    }

    // todo: WriteTo

    // 在 lua 中注册 全局的 Data 创建函数
    inline void Register(lua_State *const &L) {
        SetGlobalCClosure(L, "NewXxRandom1", [](auto L) -> int { return PushUserdata<xx::Random1>(L); });
        SetGlobalCClosure(L, "NewXxRandom2", [](auto L) -> int { return PushUserdata<xx::Random2>(L); });
        SetGlobalCClosure(L, "NewXxRandom3", [](auto L) -> int { return PushUserdata<xx::Random3>(L); });
        SetGlobalCClosure(L, "NewXxRandom4", [](auto L) -> int { return PushUserdata<xx::Random4>(L); });
    }
}
