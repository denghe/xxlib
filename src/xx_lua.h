#pragma once
#ifndef ONELUA_MAKE_LIB
extern "C" {
#endif
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#ifndef ONELUA_MAKE_LIB
}
#endif
#include "xx_string.h"

// 与 lua 交互的代码应 Try 执行, 方能正确响应 luaL_error 或 C++ 异常
// luajit 有限支持 C++ 异常, 支持 中文变量名, 官方 lua 5.3/4 很多预编译库默认不支持, 必须强制以 C++ 方式自己编译
// 注意：To 系列 批量操作时，不支持负数 idx
// 注意：这里使用 LUA_VERSION_NUM == 501 来检测是否为 luajit


// Debug 模式下，在 to 操作中启用类型检查, Release 不启用。可以修改为始终启用
#ifndef NDEBUG
#   define XX_LUA_TO_ENABLE_TYPE_CHECK 1
#else
#   define XX_LUA_TO_ENABLE_TYPE_CHECK 0
#endif


// 启用该宏，lua 低版本不支持 u/int64 的将使用 double 读写 而非 userdata(size=8)
// 15 位精度 百万亿，业务逻辑需注意. 超出将丢失低位
#define XX_LUA51_ENABLE_INT64_PUSHTO_DOUBLE false
// 用于路由 Push/To 以实现 lua 低版本 强制使用 double
using di64_t = int64_t;
using du64_t = uint64_t;


namespace xx::Lua {

	/****************************************************************************************/
	/****************************************************************************************/
	// lua push 基础适配模板. 返回实际入栈的参数个数. 可能抛 lua 异常
	// lua to 基础适配模板. 一些类型可能需要写多份适配( 填充版, 移动版, 指针版等 ). 可能抛 lua 异常
	template<typename T, typename ENABLED = void>
	struct PushToFuncs {
        // 直接通过 T 来推断 CheckStack 的值
        static constexpr int checkStackSize = 1;

        // 压数据到 lua_State, 返回长度
		static int Push_(lua_State* const& L, T&& in) {
			auto&& tn = TypeName<T>();
			CoutN("Push not found match template. type = ", tn);
			assert(false);
			return 0;
		}

        // 从 lua_State 读出指定下标的值存到 out
		static void To_(lua_State* const& L, int const& idx, T& out) {
			auto&& tn = TypeName<T>();
			CoutN("To not found match template. type = ", tn);
			assert(false);
		}
	};

    // 算多个参数需要 check 多大的 stack size
    template <std::size_t I = 0, typename...Args>
    static constexpr int CalcCheckStackSize() {
        if constexpr (I == sizeof...(Args)) return 0;
        else return PushToFuncs<std::tuple_element_t<I, std::tuple<Args...>>>::checkStackSize + CalcCheckStackSize<I + 1, Args...>();
    }

    // 堆栈空间确保。如果是 luajit 就啥都不用做了
    inline int CheckStack(lua_State* const& L, int const& n) {
#if LUA_VERSION_NUM == 501
        return 1;
#else
        return lua_checkstack(L, n);
#endif
    }

    // 参数压栈并返回实际压栈个数
	template<typename...Args>
	int Push(lua_State* const& L, Args &&...args) {
		if constexpr (sizeof...(Args) == 0) return 0;
		else {
            CheckStack(L, CalcCheckStackSize<0, Args...>());
            int n = 0;
            ((n += PushToFuncs<Args>::Push_(L, std::forward<Args>(args)), 0), ...);
            return n;
		}
	}

	// 从栈读数据 填充到变量。多参模式不支持负数 idx. 转正: lua_gettop(L) + 1 - idx
	template<typename...Args>
	void To(lua_State* const& L, int idx, Args &...args) {
		if constexpr (sizeof...(args) > 1) {
			if (idx < 0) {
				luaL_error(L, "does not support negative idx when more than 1 args");
			}
		}
		(PushToFuncs<Args>::To_(L, idx++, args), ...);
	}

	// 从栈读数据 返回( copy )
	template<typename T>
	T To(lua_State* const& L, int const& idx = 1) {
		T v;
		xx::Lua::To(L, idx, v);
		return v;
	}

	/****************************************************************************************/
	/****************************************************************************************/
	// 一些基础类型和工具函数

	// for easy push
	struct NilType {
	};

	// for easy use
	template<typename...Args>
	int Error(lua_State* const& L, Args &&... args) {
		auto str = ToString(std::forward<Args>(args)...);
		return luaL_error(L, str.c_str());
	}

	// 触发 lua 垃圾回收
	inline void GC(lua_State* const& L) {
		luaL_loadstring(L, "collectgarbage(\"collect\")");
		lua_call(L, 0, 0);
	}

	// 检查 top 值是否符合预期. args 传附加 string 信息
	template<typename...Args>
	void AssertTop(lua_State* const& L, int const& n, Args &&...args) {
		auto top = lua_gettop(L);
		if (top != n) Error(L, "AssertTop( ", std::to_string(n), " ) failed! current top = ", std::to_string(top), std::forward<Args>(args)...);
	}

	// 向 idx 的 table 写入 k, v
	template<typename K, typename V>
	void SetField(lua_State* const& L, int const& idx, K&& k, V&& v) {
		Push(L, std::forward<K>(k), std::forward<V>(v));    // ..., table at idx, ..., k, v
		lua_rawset(L, idx);                                 // ..., table at idx, ...
	}

	// 向 栈顶 的 table 写入 k, v
	template<typename K, typename V>
	void SetField(lua_State* const& L, K&& k, V&& v) {
		Push(L, std::forward<K>(k), std::forward<V>(v));    // ..., table, k, v
		lua_rawset(L, -3);                                  // ..., table, 
	}

	// 根据 k 从 idx 的 table 读出 v
	template<typename K, typename V>
	void GetField(lua_State* const& L, int idx, K const& k, V& v) {
        auto top = lua_gettop(L);
        if (idx < 0) {
            idx = top + idx + 1;
        }
        Push(L, k);											// ..., table at idx, ..., k
		lua_rawget(L, idx);									// ..., table at idx, ..., v
		To(L, top + 1, v);
		lua_settop(L, top);									// ..., table at idx, ...
	}
    template<typename V, typename K>
    V GetField(lua_State* const& L, int const& idx, K const& k) {
        V v;
        GetField(L, idx, k, v);
        return v;
    }


    // 写 k, v 到全局
	template<typename K, typename V>
	void SetGlobal(lua_State* const& L, K const& k, V&& v) {
		int n = Push(L, std::forward<V>(v));
		assert(n == 1);
		if constexpr (std::is_same_v<K, std::string> || std::is_same_v<K, std::string_view>) {
			lua_setglobal(L, k.c_str());
		}
		else {
			lua_setglobal(L, k);
		}
	}

	// 从全局以 k 读 v
	template<typename K, typename V>
	void GetGlobal(lua_State* const& L, K const& k, V& v) {
		auto top = lua_gettop(L);
		if constexpr (xx::IsContainer_v<K>) {
			lua_getglobal(L, k.data());
		}
		else {
			lua_getglobal(L, k);
		}
		To(L, top + 1, v);
		lua_settop(L, top);
	}

	template<typename R, typename K>
	R GetGlobal(lua_State* const& L, K const& k) {
		R r;
		GetGlobal(L, k, r);
		return r;
	}


	// 写 k, v 到注册表
	template<typename K, typename V>
	void SetRegistry(lua_State* const& L, K const& k, V&& v) {
		SetField(L, LUA_REGISTRYINDEX, k, std::forward<V>(v));
	}

	// 从注册表以 k 读 v
	template<typename K, typename V>
	void GetRegistry(lua_State* const& L, K const& k, V const& v) {
		GetField(L, LUA_REGISTRYINDEX, k, v);
	}

	// 加载文件
	template<typename T>
	void LoadFile(lua_State* const& L, T&& fileName) {
		int rtv = 0;
		if constexpr (std::is_same_v<std::string, std::remove_const_t<std::remove_reference_t<T>>>) {
			rtv = luaL_loadfile(L, fileName.c_str());
		}
		else {
			rtv = luaL_loadfile(L, fileName);
		}
		if (rtv != LUA_OK) {
			lua_error(L);
		};
	}

	namespace Detail {
		// Exec / PCall 返回值封装, 易于使用
		struct Result {
			Result() = default;
			Result(Result const&) = default;
			Result& operator=(Result const&) = default;
			Result(Result&& o) = default;
			Result& operator=(Result&& o) = default;

			int n = 0;
			std::string m;

			template<typename M>
			Result(int const& n, M&& m) : n(n), m(std::forward<M>(m)) {}

			inline operator bool() const {
				return n != 0;
			}
		};

		// for pcall
		inline Result PCallCore(lua_State* const& L, int const& top, int const& n) {
			Result rtv;
			if ((rtv.n = lua_pcall(L, n, LUA_MULTRET, 0))) {                // ... ( func? errmsg? )
				if (lua_isstring(L, -1)) {
					rtv.m = lua_tostring(L, -1);
				}
				else if (rtv.n == -1) {
					rtv.m = "cpp exception( can edit ldo.c LUAI_TRY macro for more info )";
				}
				else {
					rtv.m = "lua_error forget arg?";
				}
				lua_settop(L, top);
			}
			return rtv;
		}
	}

	// 安全调用函数( 函数最先压栈，然后是 up values ). 出错将还原 top
	// [-(nargs + 1), +(nresults|1), -]
	template<typename...Args>
	Detail::Result PCall(lua_State* const& L, Args &&...args) {
		auto top = lua_gettop(L);
		int n = 0;
		if constexpr (sizeof...(args) > 0) {
			n = Push(L, std::forward<Args>(args)...);
		}
		return Detail::PCallCore(L, top, n);
	}

	// 安全执行 lambda。出错将还原 top
	template<typename T>
	Detail::Result Try(lua_State* const& L, T&& func) {
		if (!CheckStack(L, 2)) return { -2, "lua_checkstack(L, 2) failed." };
		auto top = lua_gettop(L);
		lua_pushlightuserdata(L, &func);                                // ..., &func
		lua_pushcclosure(L, [](auto L) {                                // ..., cfunc
			auto&& f = (T*)lua_touserdata(L, lua_upvalueindex(1));
			(*f)();
			return 0;
			}, 1);
		return Detail::PCallCore(L, top, 0);
	}

	// 安全调用指定名称的全局函数( 会留下函数和返回值在栈中 )
	// [-(nargs + 1), +(nresults|1), -]
	template<typename T, typename...Args>
	Detail::Result PCallGlobalFunc(lua_State* const& L, T&& funcName, Args &&...args) {
		if constexpr (std::is_same_v<std::string, std::remove_const_t<std::remove_reference_t<T>>>) {
			lua_getglobal(L, funcName.c_str());
		}
		else {
			lua_getglobal(L, funcName);
		}
		return PCall(L, std::forward<Args>(args)...);
	}

	// 不安全调用栈顶函数
	// [-(nargs + 1), +nresults, e]
	template<typename...Args>
	void Call(lua_State* const& L, Args &&...args) {
		lua_call(L, Push(L, std::forward<Args>(args)...), LUA_MULTRET);
	}

	// 不安全调用指定名称的全局函数( 会留下函数和返回值在栈中 )
	// [-(nargs + 1), +(nresults|1), e]
	template<typename T, typename...Args>
	void CallGlobalFunc(lua_State* const& L, T&& funcName, Args &&...args) {
        CheckStack(L, 1);
		if constexpr (xx::IsContainer_v<T>) {
			lua_getglobal(L, funcName.data());
		}
		else {
			lua_getglobal(L, funcName);
		}
		Call(L, std::forward<Args>(args)...);
	}

    // 不安全调用指定名称的全局函数, 并返回 R( 会还原栈 )
    // [-(nargs + 1), +(nresults|1), e]
    template<typename R = void, typename T, typename...Args>
    R CallGlobalFuncR(lua_State* const& L, T&& funcName, Args &&...args) {
        auto top = lua_gettop(L);
        CallGlobalFunc(L, std::forward<T>(funcName), std::forward<Args>(args)...);
        if constexpr (!std::is_void_v<R>) {
            R rtv;
            To(L, top + 1, rtv);
            lua_settop(L, top);											// ...
            return rtv;
        }
        else {
            lua_settop(L, top);											// ...
        }
    }


	template<typename T, typename...Args>
	void DoFile(lua_State* const& L, T&& fileName, Args &&...args) {
		LoadFile(L, std::forward<T>(fileName));
		Call(L, std::forward<Args>(args)...);
	}

	template<typename...Args>
	void DoString(lua_State* const& L, char const* const& code, Args &&...args) {
		if (LUA_OK != luaL_loadstring(L, code)) {
			lua_error(L);
		}
		Call(L, std::forward<Args>(args)...);
	}

	template<typename...Args>
	void DoBuffer(lua_State* const& L, std::string_view code, std::string const& name, Args &&...args) {
		if (LUA_OK != luaL_loadbufferx(L, code.data(), code.size(), name.c_str(), nullptr)) {
			lua_error(L);
		}
		Call(L, std::forward<Args>(args)...);
	}

	// Lua State 简单封装, 可直接当指针用, 离开范围自动 close
	struct State {
		lua_State* L = nullptr;

		operator lua_State* () {
			return L;
		}

		~State() {
			lua_close(L);
		}

		explicit State(bool const& openLibs = true) {
			L = luaL_newstate();
			if (!L) throw std::runtime_error("auto &&L = luaL_newstate(); if (!L)");
			if (openLibs) {
				if (auto r = Try(L, [&] { luaL_openlibs(L); })) {
					throw std::runtime_error(r.m);
				}
			}
		}

		explicit State(lua_State* L) : L(L) {}
		State(State const&) = delete;
		State& operator=(State const&) = delete;
		State(State&& o) noexcept : L(o.L) { o.L = nullptr; }
		State& operator=(State&& o) noexcept {
			std::swap(L, o.L);
			return *this;
		}
	};

	// 带扩展数据的 lua_State 的简单封装
	template<typename T>
	struct StateWithExtra : State {
		using State::State;
		T extra{};
		explicit StateWithExtra(bool const& openLibs = true) : State(openLibs) {
			memcpy(lua_getextraspace(L), &extra, sizeof(void*));
		}
		static T& Extra(lua_State* L) {
			return *(T*)lua_getextraspace(L);
		}
	};

	/****************************************************************************************/
	/****************************************************************************************/
	// 各种 Push To 适配

	/******************************************************/
	// 适配 nil 写入
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<NilType, std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, NilType &&) {
			lua_pushnil(L);
			return 1;
		}
	};

	/******************************************************/
	// 适配 整数( bool 也算 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_integral_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T && in) {
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
				lua_pushboolean(L, in ? 1 : 0);
			}
			else {
#if LUA_VERSION_NUM == 501
			    if constexpr (sizeof(T) == 8) {
                    if constexpr (XX_LUA51_ENABLE_INT64_PUSHTO_DOUBLE || std::is_same_v<std::decay_t<T>, di64_t> || std::is_same_v<std::decay_t<T>, du64_t>) {
					    lua_pushnumber(L, (double)in);
					} else {
                        *(std::decay_t<T>*)lua_newuserdata(L, 8) = in;
                    }
			    }
			    else
#endif
                {
                    lua_pushinteger(L, in);
                }
			}
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
				if (!lua_isboolean(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not bool");
#endif
				out = (bool)lua_toboolean(L, idx);
			}
			else {
#if LUA_VERSION_NUM == 501
                if constexpr (sizeof(T) == 8) {
                    if constexpr (XX_LUA51_ENABLE_INT64_PUSHTO_DOUBLE || std::is_same_v<std::decay_t<T>, di64_t> || std::is_same_v<std::decay_t<T>, du64_t>) {
                        int isnum = 0;
                        out = (T)lua_tonumberx(L, idx, &isnum);
                        if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
                    } else {
                        if (!lua_isuserdata(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not number(int64 userdata)");
                        out = *(std::decay_t<T>*)lua_touserdata(L, idx);
                    }
				}
			    else
#endif
                {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
                    int isnum = 0;
                    out = (T) lua_tointegerx(L, idx, &isnum);
                    if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
#else
					out = (T)lua_tointeger(L, idx);
#endif
                }

			}
		}
	};

	/******************************************************/
	// 适配 枚举( 转为整数 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_enum_v<std::decay_t<T>>>> {
		typedef std::underlying_type_t<std::decay_t<T>> UT;
        static constexpr int checkStackSize = ::xx::Lua::PushToFuncs<UT>::checkStackSize;
		static int Push_(lua_State* const& L, T && in) {
			return ::xx::Lua::PushToFuncs<UT>::Push_(L, (UT)in);
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			::xx::Lua::PushToFuncs<UT>::To_(L, idx, (UT&)out);
		}
	};

	/******************************************************/
	// 适配 浮点
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T && in) {
			lua_pushnumber(L, in);
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			int isnum = 0;
			out = (T)lua_tonumberx(L, idx, &isnum);
			if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
#else
			out = (T)lua_tonumber(L, idx);
#endif
		}
	};

	/******************************************************/
	// 适配 literal char[len] string 写入
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T && in) {
			lua_pushlstring(L, in, IsLiteral<T>::len - 1);
			return 1;
		}
	};

	// 适配 char*, char const* 写入
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, char*> || std::is_same_v<std::remove_reference_t<T>, char const*>>> {
        static constexpr int checkStackSize = 1;
        static int Push_(lua_State* const& L, T && in) {
			lua_pushstring(L, in);
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
#endif
			out = (T)lua_tostring(L, idx);
		}
	};

	// 适配 std::string_view
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
        static int Push_(lua_State* const& L, T && in) {
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
#endif
			size_t len = 0;
			auto buf = lua_tolstring(L, idx, &len);
			out = std::string_view(buf, len);
		}
	};

	/******************************************************/
	// 适配 std::string ( copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
#endif
			size_t len = 0;
			auto ptr = lua_tolstring(L, idx, &len);
			out.assign(ptr, len);
		}
	};

	/******************************************************/
	// 适配 void*
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, void*>>> {
        static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T && in) {
			lua_pushlightuserdata(L, in);
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_islightuserdata(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not void*");
#endif
			out = (T)lua_touserdata(L, idx);
		}
	};

	/******************************************************/
	// 适配 std::optional ( move or copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsOptional_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = ::xx::Lua::PushToFuncs<typename std::decay_t<T>::value_type, void>::checkStackSize;
		static int Push_(lua_State* const& L, T&& in) {
			if (in.has_value()) {
			    if constexpr (std::is_rvalue_reference_v<decltype(in)>) {
                    Push(L, std::move(in.value()));
			    }
			    else {
                    Push(L, in.value());
                }
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			if (lua_isnil(L, idx)) {
				out.reset();
			}
			else {
			    out = std::move(To<typename std::decay_t<T>::value_type>(L, idx));
			}
		}
	};

	/******************************************************/
	// 适配 std::pair ( 值展开 move or copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<xx::IsPair_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = ::xx::Lua::PushToFuncs<typename std::decay_t<T>::first_type>::checkStackSize + ::xx::Lua::PushToFuncs<typename std::decay_t<T>::second_type>::checkStackSize;
		static int Push_(lua_State* const& L, T && in) {
            if constexpr (std::is_rvalue_reference_v<decltype(in)>) {
                return Push(L, std::move(in.first), std::move(in.second));
            }
            else {
                return Push(L, in.first, in.second);
            }
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			To(L, idx, out.first, out.second);
		}
	};

	/******************************************************/
	// 适配 std::tuple ( 值展开 move or copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsTuple_v<std::decay_t<T>>>> {
        template <std::size_t I = 0>
        static constexpr int CalcCheckStackSize() {
            if constexpr (I == std::tuple_size_v<T>) return 0;
            else return ::xx::Lua::PushToFuncs<std::tuple_element_t<I, T>>::checkStackSize + CalcCheckStackSize<I + 1>();
        }
        static constexpr int checkStackSize = CalcCheckStackSize();
        static int Push_(lua_State* const& L, T && in) {
			int rtv = 0;
			std::apply([&](auto &... args) {
                if constexpr (std::is_rvalue_reference_v<decltype(in)>) {
                    rtv = Push(L, std::move(args)...);
                }
                else {
                    rtv = Push(L, args...);
                }
				}, in);
			return rtv;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			std::apply([&](auto &... args) {
				To(L, idx, args...);
				}, out);
		}
	};
	template<>
	struct PushToFuncs<std::tuple<>, void> {
		static int Push_(lua_State* const& L, std::tuple<> && in) {
			return 0;
		}
		static void To_(lua_State* const& L, int const& idx, std::tuple<>& out) {
		}
	};

	/******************************************************/
	// 适配 std::vector<T>. 体现为 table ( copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsVector_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = 4;
        static int Push_(lua_State* const& L, T && in) {
			auto siz = (int)in.size();
			lua_createtable(L, siz, 0);
			for (int i = 0; i < siz; ++i) {
                SetField(L, i + 1, in[i]);
			}
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", TypeName<T>());
#endif
			out.clear();
			int top = lua_gettop(L) + 1;
			CheckStack(L, 4);
			out.reserve(32);
			for (lua_Integer i = 1;; i++) {
				lua_rawgeti(L, idx, i);									// ... t(idx), ..., val/nil
				if (lua_isnil(L, top)) {
					lua_pop(L, 1);										// ... t(idx), ...
					return;
				}
				typename T::value_type v;
				To(L, top, v);
				lua_pop(L, 1);											// ... t(idx), ...
				out.emplace_back(std::move(v));
			}
		}
	};

	/******************************************************/
	// 适配 std::[unordered_]map<K, V>. 体现为 table ( copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsMapSeries_v<std::decay_t<T>>>> {
        static constexpr int checkStackSize = 4;
		static int Push_(lua_State* const& L, T && in) {
			lua_createtable(L, 0, (int)in.size());
			for (auto&& kv : in) {
				SetField(L, kv.first, kv.second);
			}
			return 1;
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			out.clear();
#if XX_LUA_TO_ENABLE_TYPE_CHECK
			if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", TypeName<T>());
#endif
			int top = lua_gettop(L);
			CheckStack(L, 4);
			lua_pushnil(L);                                             // ... t(idx), ..., nil
			while (lua_next(L, idx) != 0) {                             // ... t(idx), ..., k, v
				typename T::key_type k;
				To(L, top + 1, k);
				typename T::mapped_type v;
				To(L, top + 2, v);
				out.emplace(std::move(k), std::move(v));
				lua_pop(L, 1);                                         // ... t(idx), ..., k
			}
		}
	};

	// todo: more 适配
}
