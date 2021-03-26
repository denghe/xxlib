#pragma once
#include "xx_ptr.h"
#ifndef MAKE_LIB
extern "C" {
#endif
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#ifndef MAKE_LIB
}
#endif

namespace xx::Lua {

	/****************************************************************************************/
	/****************************************************************************************/
	// lua push 基础适配模板. 返回实际入栈的参数个数. 可能抛 lua 异常
	template<typename T, typename ENABLED = void>
	struct PushFuncs {
		static int Push(lua_State* const& L, T&& in) {
			assert(false);
			return 0;
		}
	};

	// 封装为易用函数
	template<typename...Args>
	inline int Push(lua_State* const& L, Args &&...args) {
		return (... + PushFuncs<Args>::Push(L, std::forward<Args>(args)));
	}

	/****************************************************************************************/
	/****************************************************************************************/
	// lua to 基础适配模板. 一些类型可能需要写多份适配( 填充版, 移动版, 指针版等 ). 可能抛 lua 异常
	template<typename T, typename ENABLED = void>
	struct ToFuncs {
		static void To(lua_State* const& L, int const& idx, T& out) {
			assert(false);
		}
	};


	// 多参模式不支持负数 idx. 转正: lua_gettop(L) + 1 - idx
	template<typename...Args>
	inline void To(lua_State* const& L, int idx, Args &...args) {
		if constexpr (sizeof...(args) > 1) {
			if (idx < 0) {
				luaL_error(L, "does not support negative idx when more than 1 args");
			}
		}
		(ToFuncs<Args>::To(L, idx++, args), ...);
	}

	// 从指定位置读指定类型并返回
	template<typename T>
	inline T To(lua_State* const& L, int const& idx = 1) {
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

	// 堆栈空间确保。如果是 luajit 就啥都不用做了
	inline int CheckStack(lua_State* const& L, int const& n) {
#if LUA_VERSION_NUM == 501
		return 1;
#else
		return lua_checkstack(L, n);
#endif
	}

	// for easy use
	template<typename...Args>
	inline int Error(lua_State* const& L, Args &&... args) {
		return luaL_error(L, (... + args).c_str());
	}


	/****************************************************************************************/
	/****************************************************************************************/
	// lua 向 c++ 注册的回调函数的封装, 将函数以 luaL_ref 方式放入注册表
	// 通常被 lambda 捕获携带, 析构时同步删除 lua 端函数。 需确保比 L 早死, 否则 L 就野了
	struct FuncWrapper {
		FuncWrapper() = default;
		FuncWrapper(FuncWrapper const& o) = default;
		FuncWrapper& operator=(FuncWrapper const& o) = default;
		FuncWrapper(FuncWrapper&& o) = default;
		FuncWrapper& operator=(FuncWrapper&& o) = default;

		Shared<std::pair<lua_State*, int>> p;

		FuncWrapper(lua_State* const& L, int const& idx) {
			if (!lua_isfunction(L, idx)) Error(L, "args[", std::to_string(idx), "] is not a lua function");
			CheckStack(L, 1);
			p.Emplace();
			p->first = L;
			lua_pushvalue(L, idx);                                      // ..., func, ..., func
			p->second = luaL_ref(L, LUA_REGISTRYINDEX);                 // ..., func, ...
		}

		// 如果 p 引用计数唯一, 则反注册
		~FuncWrapper() {
			if (p.useCount() != 1) return;
			luaL_unref(p->first, LUA_REGISTRYINDEX, p->second);
		}
	};


	/****************************************************************************************/
	/****************************************************************************************/
	// 各种 Push To 适配

	/******************************************************/
	// 适配 nil 写入
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<NilType, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, NilType const&) {
			CheckStack(L, 1);
			lua_pushnil(L);
			return 1;
		}
	};

	/******************************************************/
	// 适配 整数( bool 也算 )
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_integral_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
				lua_pushboolean(L, in ? 1 : 0);
			}
			else {
				// todo: 兼容 int64 的处理
				lua_pushinteger(L, in);
			}
			return 1;
		}
	};

	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_integral_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
				if (!lua_isboolean(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not bool");
				out = (bool)lua_toboolean(L, idx);
			}
			else {
				// todo: 兼容 int64 的处理
				int isnum = 0;
				out = (T)lua_tointegerx(L, idx, &isnum);
				if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
			}
		}
	};

	/******************************************************/
	// 适配 枚举( 转为整数 )
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_enum_v<std::decay_t<T>>>> {
		typedef std::underlying_type_t<T> UT;
		static inline int Push(lua_State* const& L, T const& in) {
			return PushFuncs<UT, void>::Push(L, (UT)in);
		}
	};

	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_enum_v<std::decay_t<T>>>> {
		typedef std::underlying_type_t<T> UT;
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			ToFuncs<UT, void>::To(L, (UT&)out);
		}
	};

	/******************************************************/
	// 适配 浮点
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			lua_pushnumber(L, in);
			return 1;
		}
	};

	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			int isnum = 0;
			out = (T)lua_tonumberx(L, idx, &isnum);
			if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
		}
	};

	/******************************************************/
	// 适配 literal char[len] string 写入
	template<size_t len>
	struct PushFuncs<char[len], void> {
		static inline int Push(lua_State* const& L, char const(&in)[len]) {
			CheckStack(L, 1);
			lua_pushlstring(L, in, len - 1);
			return 1;
		}
	};

	// 适配 char*, char const* 写入
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, char*> || std::is_same_v<std::remove_reference_t<T>, char const*>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			lua_pushstring(L, in);
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
			out = (T)lua_tostring(L, idx);
		}
	};

	// 适配 std::string_view
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::string_view const& in) {
			CheckStack(L, 1);
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}
	};

	// 不复制数据. 需立刻使用
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, std::string_view& out) {
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
			size_t len = 0;
			auto buf = lua_tolstring(L, idx, &len);
			out = std::string_view(buf, len);
		}
	};

	/******************************************************/
	// 适配 std::string
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::string const& in) {
			CheckStack(L, 1);
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}
	};

	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, std::string& out) {
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
			size_t len = 0;
			auto ptr = lua_tolstring(L, idx, &len);
			out.assign(ptr, len);
		}
	};

	/******************************************************/
	// 适配 void*
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, void*>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			lua_pushlightuserdata(L, in);
			return 1;
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, void*>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if (!lua_islightuserdata(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not void*");
			out = (T)lua_touserdata(L, idx);
		}
	};

	/******************************************************/
	// 适配 std::optional
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<IsOptional_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::optional<T>&& in) {
			CheckStack(L, 1);
			if (in.has_value()) {
				PushFuncs<T>::Push(in.value());
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<IsOptional_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, std::optional<T>& out) {
			if (lua_isnil(L, idx)) {
				out.reset();
			}
			else {
				ToFuncs<T>::To(L, idx, out.value());
			}
		}
	};

	/******************************************************/
	// 适配 std::pair ( 值展开 )
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<xx::IsPair_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			return ::xx::Lua::Push(L, in.first, in.second);
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<xx::IsPair_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			::xx::Lua::To(L, idx, out.first, out.second);
		}
	};

	/******************************************************/
	// 适配 std::tuple ( 值展开 )
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<xx::IsTuple_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			int rtv = 0;
			std::apply([&](auto &... args) {
				rtv = ::xx::Lua::Push(L, args...);
				}, in);
			return rtv;
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<xx::IsTuple_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			std::apply([&](auto &... args) {
				::xx::Lua::To(L, idx, args...);
				}, out);
		}
	};

	template<>
	struct PushFuncs<std::tuple<>, void> {
		static inline int Push(lua_State* const& L, std::tuple<> const& in) {
			return 0;
		}
	};
	template<>
	struct ToFuncs<std::tuple<>, void> {
		static inline void To(lua_State* const& L, int const& idx, std::tuple<>& out) {
		}
	};


	/******************************************************/
	// 适配 std::vector<T>. 体现为 table
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<IsVector_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			auto siz = (int)in.size();
			lua_createtable(L, siz, 0);
			for (int i = 0; i < siz; ++i) {
				SetField(L, i + 1, in[i]);
			}
			return 1;
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<IsVector_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", std::string(xx::TypeName_v<T>));
			out.clear();
			int top = lua_gettop(L) + 1;
			Lua::CheckStack(L, 2);
			out.reserve(32);
			for (lua_Integer i = 1;; i++) {
				lua_rawgeti(L, idx, i);									// ... t(idx), ..., val/nil
				if (lua_isnil(L, top)) {
					lua_pop(L, 1);										// ... t(idx), ...
					return;
				}
				typename T::value_type v;
				::xx::Lua::To(L, top, v);
				lua_pop(L, 1);											// ... t(idx), ...
				out.emplace_back(std::move(v));
			}
		}
	};

	/******************************************************/
	// 适配 std::[unordered_]map<K, V>. 体现为 table
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<IsMapSeries_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			lua_createtable(L, 0, (int)in.size());
			for (auto&& kv : in) {
				SetField(L, kv.first, kv.second);
			}
			return 1;
		}
	};
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<IsMapSeries_v<std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			out.clear();
			if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", xx::TypeName_v<T>);
			int top = lua_gettop(L);
			Lua::CheckStack(L, 1);
			lua_pushnil(L);                                             // ... t(idx), ..., nil
			while (lua_next(L, idx) != 0) {                             // ... t(idx), ..., k, v
				typename T::key_type k;
				::xx::Lua::To(L, top + 1, k);
				typename T::value_type v;
				::xx::Lua::To(L, top + 2, v);
				out.emplace(std::move(k), std::move(v));
				lua_pop(L, 1);                                         // ... t(idx), ..., k
			}
		}
	};

	// todo: more 适配
}
