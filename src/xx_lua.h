#pragma once

// 与 lua 交互的代码应 Try 执行, 方能正确响应 luaL_error 或 C++ 异常
// luajit 有限支持 C++ 异常, 支持 中文变量名, 官方 lua 5.3/4 很多预编译库默认不支持, 必须强制以 C++ 方式自己编译
// 注意：To 系列 批量操作时，不支持负数 idx ( 递归模板里 + 1 就不对了 )
// 注意：这里使用 LUA_VERSION_NUM == 501 来检测是否为 luajit

#include "xx_helpers.h"
#include "xx_data.h"

#ifndef MAKE_LIB
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifndef MAKE_LIB
}
#endif

// 设置是否使用 lua_newthread 创建的 lua_State 来存放 函数 以加速调用( 但是创建性能剧烈降低 )
//#define USE_NEW_THREAD_HOLD_FUNC


namespace xx::Lua {

	// for easy use
	template<typename...Args>
	inline int Error(lua_State* const& L, Args &&... args) {
		return luaL_error(L, (args + ...).c_str());
	}

	// for easy push
	struct NilType {
	};

	// 如果是 luajit 就啥都不用做了
	inline int CheckStack(lua_State* const& L, int const& n) {
#if LUA_VERSION_NUM == 501
		return 1;
#else
		return lua_checkstack(L, n);
#endif
	}


	/******************************************************************************************************************/
	// 类型 metatable 填充函数适配模板
	template<typename T, typename ENABLED = void>
	struct MetaFuncs {
		inline static std::string name = std::string(TypeName_v<T>);

		static void Fill(lua_State* const& L);// {
			//Meta<T>(L, name)
				//.Func("xxxx", &T::xxxxxx)
				//.Prop("GetXxxx", "SetXxxx", &T::xxxx)
				//.Lambda("Xxxxxx", [](T& self, ...) { ... });
		//}
	};

	// 压入指定类型的 metatable( 以 MetaFuncs<T>::name.data() 为 key, 存放与注册表。没有找到就创建并放入 )
	template<typename T>
	void PushMeta(lua_State* const& L) {
		CheckStack(L, 3);
#if LUA_VERSION_NUM == 501
		lua_pushlightuserdata(L, (void*)MetaFuncs<T>::name.data());         // ..., key
		lua_rawget(L, LUA_REGISTRYINDEX);                                   // ..., mt?
#else
		lua_rawgetp(L, LUA_REGISTRYINDEX, MetaFuncs<T>::name.data());       // ..., mt?
#endif
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);                                                  // ...
			CheckStack(L, 4);
			lua_createtable(L, 0, 20);                                      // ..., mt

			// 如果类型需要析构就自动生成 __gc
			if constexpr (std::is_destructible_v<T>) {
				lua_pushstring(L, "__gc");                                  // ..., mt, "__gc"
				lua_pushcclosure(L, [](auto L)->int {                       // ..., mt, "__gc", cc
					using U = T;
					auto f = (U*)lua_touserdata(L, -1);
					f->~U();
					return 0;
					}, 0);
				lua_rawset(L, -3);                                          // ..., mt
			}

			// 如果类型不是可执行的
			if constexpr (!IsLambda_v<T> && !IsFunction_v<T>) {
				lua_pushstring(L, "__index");                               // ..., mt, "__index"
				lua_pushvalue(L, -2);                                       // ..., mt, "__index", mt
				lua_rawset(L, -3);                                          // ..., mt
			}

			MetaFuncs<T, void>::Fill(L);                                    // ..., mt

#if LUA_VERSION_NUM == 501
			lua_pushlightuserdata(L, (void*)MetaFuncs<T>::name.data());            // ..., mt, key
			lua_pushvalue(L, -2);                                           // ..., mt, key, mt
			lua_rawset(L, LUA_REGISTRYINDEX);                               // ..., mt
#else
			lua_pushvalue(L, -1);                                           // ..., mt, mt
			lua_rawsetp(L, LUA_REGISTRYINDEX, MetaFuncs<T>::name.data());   // ..., mt
#endif
			//std::cout << "PushMeta MetaFuncs<T>::name.data() = " << (size_t)MetaFuncs<T>::name.data() << " T = " << MetaFuncs<T>::name << std::endl;
		}
	}

	/******************************************************************************************************************/
	// Lua push, to 系列基础适配模板. Push 返回实际入栈的参数个数( 通常是 1. 但如果传入一个队列并展开压栈则不一定 ). To 无返回值.
	// 可能抛 lua 异常( 故这些函数应该间接被 pcall 使用 )
	template<typename T, typename ENABLED = void>
	struct PushToFuncs {
		static int Push(lua_State* const& L, T&& in);

		template<typename U = std::decay_t<T>>
		static void ToPtr(lua_State* const& L, int const& idx, U*& out);

		static void To(lua_State* const& L, int const& idx, T& out);
	};

	// 适配 nil 写入
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<NilType, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, NilType const& in) {
			CheckStack(L, 1);
			lua_pushnil(L);
			return 1;
		}
	};

	// 适配 整数( bool 也算 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_integral_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
				lua_pushboolean(L, in ? 1 : 0);
			}
			else {
				lua_pushinteger(L, in);
			}
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
				if (!lua_isboolean(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not bool");
				out = (bool)lua_toboolean(L, idx);
			}
			else {
				int isnum = 0;
				out = (T)lua_tointegerx(L, idx, &isnum);
				if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
			}
		}
	};

	// 适配 枚举( 转为整数 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_enum_v<std::decay_t<T>>>> {
		typedef std::underlying_type_t<T> UT;

		static inline int Push(lua_State* const& L, T const& in) {
			return PushToFuncs<UT, void>::Push(L, (UT)in);
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			PushToFuncs<UT, void>::To(L, (UT&)out);
		}
	};

	// 适配 浮点
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			lua_pushnumber(L, in);
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			int isnum = 0;
			out = (T)lua_tonumberx(L, idx, &isnum);
			if (!isnum) Error(L, "error! args[", std::to_string(idx), "] is not number");
		}
	};

	// 适配 literal char[len] string ( 写入 )
	template<size_t len>
	struct PushToFuncs<char[len], void> {
		static inline int Push(lua_State* const& L, char const(&in)[len]) {
			CheckStack(L, 1);
			lua_pushlstring(L, in, len - 1);
			return 1;
		}
	};

	// 适配 std::string
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::string const& in) {
			CheckStack(L, 1);
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, std::string& out) {
			if (!lua_isstring(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not string");
			size_t len = 0;
			auto ptr = lua_tolstring(L, idx, &len);
			out.assign(ptr, len);
		}
	};

	// 适配 std::string_view( 写入 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::string_view const& in) {
			CheckStack(L, 1);
			lua_pushlstring(L, in.data(), in.size());
			return 1;
		}
	};

	// 适配 char*, char const*
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, char*> || std::is_same_v<std::remove_reference_t<T>, char const*>>> {
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

	// 适配 void*
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, void*>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			CheckStack(L, 1);
			lua_pushlightuserdata(L, in);
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			if (!lua_islightuserdata(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not void*");
			out = (T)lua_touserdata(L, idx);
		}
	};

	// 适配 std::optional. 注意：value 不可以是 不能拷贝的类型，例如 unique_ptr
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsOptional_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, std::optional<T>&& in) {
			CheckStack(L, 1);
			if (in.has_value()) {
				PushToFuncs<T>::Push(in.value());
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, std::optional<T>& out) {
			if (lua_isnil(L, idx)) {
				out.reset();
			}
			else {
				PushToFuncs<T>::To(L, idx, out.value());
			}
		}
	};

	// 适配模板转为函数
	namespace Detail {
		template<typename Arg, typename...Args>
		inline int Push(lua_State* const& L, Arg&& arg) {
			return ::xx::Lua::PushToFuncs<Arg>::Push(L, std::forward<Arg>(arg));
		}

		template<typename Arg, typename...Args>
		inline int Push(lua_State* const& L, Arg&& arg, Args &&...args) {
			int n = ::xx::Lua::PushToFuncs<Arg>::Push(L, std::forward<Arg>(arg));
			return n + Push(L, std::forward<Args>(args)...);
		}

		inline int Push(lua_State* const& L) {
			return 0;
		}
	}

	template<typename...Args>
	inline int Push(lua_State* const& L, Args &&...args) {
		return ::xx::Lua::Detail::Push(L, std::forward<Args>(args)...);
	}

	inline int Push(lua_State* const& L) {
		return 0;
	}

	// 适配模板转为函数
	namespace Detail {

		template<typename Arg, typename...Args>
		inline void To(lua_State* const& L, int const& idx, Arg& arg, Args &...args) {
			xx::Lua::PushToFuncs<Arg>::To(L, idx, arg);
			if constexpr (sizeof...(Args) > 0) {
				To(L, idx + 1, args...);
			}
		}

		inline void To(lua_State* const& L, int const& idx) {
		}
	}

	template<typename...Args>
	inline void To(lua_State* const& L, int const& idx, Args &...args) {
		xx::Lua::Detail::To(L, idx, args...);
	}

	// 适配 std::tuple
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<xx::IsTuple_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			int rtv = 0;
			std::apply([&](auto &... args) {
				rtv = xx::Lua::Push(L, args...);
				}, in);
			return rtv;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			std::apply([&](auto &... args) {
				xx::Lua::To(L, idx, args...);
				}, out);
		}
	};

	template<>
	struct PushToFuncs<std::tuple<>, void> {
		static inline int Push(lua_State* const& L, std::tuple<> const& in) {
			return 0;
		}

		static inline void To(lua_State* const& L, int const& idx, std::tuple<>& out) {
		}
	};


	/******************************************************************************************************************/
	// 常用交互函数封装

	// 向 idx 的 table 写入 k, v
	template<typename K, typename V>
	inline void SetField(lua_State* const& L, int const& idx, K&& k, V&& v) {
		Push(L, std::forward<K>(k), std::forward<V>(v));    // ..., table at idx, ..., k, v
		lua_rawset(L, idx);                                 // ..., table at idx, ...
	}

	// 向 栈顶 的 table 写入 k, v
	template<typename K, typename V>
	inline void SetField(lua_State* const& L, K&& k, V&& v) {
		Push(L, std::forward<K>(k), std::forward<V>(v));    // ..., table at idx, ..., k, v
		lua_rawset(L, -3);                                  // ..., table at idx, ...
	}


	// 根据 k 从 idx 的 table 读出 v
	template<typename K, typename V>
	inline void GetField(lua_State* const& L, int const& idx, K const& k, V& v) {
		auto top = lua_gettop(L);
		Push(L, k);                             // ..., table at idx, ..., k
		lua_rawget(L, idx);                     // ..., table at idx, ..., v
		To(L, top + 1, v);
		lua_settop(L, top);                     // ..., table at idx, ...
	}

	// 写 k, v 到全局
	template<typename K, typename V>
	inline void SetGlobal(lua_State* const& L, K const& k, V&& v) {
		Push(L, std::forward<V>(v));
		if constexpr (std::is_same_v<K, std::string> || std::is_same_v<K, std::string_view>) {
			lua_setglobal(L, k.c_str());
		}
		else {
			lua_setglobal(L, k);
		}
	}

	// 从全局以 k 读 v
	template<typename K, typename V>
	inline void GetGlobal(lua_State* const& L, K const& k, V& v) {
		auto top = lua_gettop(L);
		if constexpr (std::is_same_v<K, std::string> || std::is_same_v<K, std::string_view>) {
			lua_getglobal(L, k.c_str());
		}
		else {
			lua_getglobal(L, k);
		}
		To(L, top + 1, v);
		lua_settop(L, top);
	}

	// 写 k, v 到注册表
	template<typename K, typename V>
	inline void SetRegistry(lua_State* const& L, K const& k, V&& v) {
		SetField(L, LUA_REGISTRYINDEX, k, std::forward<V>(v));
	}

	// 从注册表以 k 读 v
	template<typename K, typename V>
	inline void GetRegistry(lua_State* const& L, K const& k, V const& v) {
		GetField(L, LUA_REGISTRYINDEX, k, v);
	}

	// 压入一个 T( 内容复制到 userdata, 且注册 mt )
	template<typename T>
	int PushUserdata(lua_State* const& L, T&& v) {
		using U = std::decay_t<T>;
		CheckStack(L, 2);
		auto p = lua_newuserdata(L, sizeof(U));                         // ..., ud
		new(p) U(std::forward<T>(v));
		PushMeta<U>(L);                                                 // ..., ud, mt
		lua_setmetatable(L, -2);                                        // ..., ud
		return 1;
	}

	// 适配 lambda
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<xx::IsLambda_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T&& in) {
			PushUserdata(L, std::forward<T>(in));                       // ..., ud
			lua_pushcclosure(L, [](auto L) {                            // ..., cc
				auto f = (T*)lua_touserdata(L, lua_upvalueindex(1));
				FuncA_t<T> tuple;
				To(L, 1, tuple);
				int rtv = 0;
				std::apply([&](auto &... args) {
					if constexpr (std::is_void_v<FuncR_t<T>>) {
						(*f)(args...);
					}
					else {
						rtv = xx::Lua::Push(L, (*f)(args...));
					}
					}, tuple);
				return rtv;
				}, 1);
			return 1;
		}
	};

	// 被 std::function 捕获携带, 当捕获列表析构发生时, 自动从 L 中反注册函数
	// 需自己确保这些 function 活的比 L 久
	struct FuncWrapper {
		// 将函数以 luaL_ref 方式放入注册表.
		std::shared_ptr<std::pair<lua_State*, int>> p;

		FuncWrapper() = default;

		FuncWrapper(FuncWrapper const& o) = default;

		FuncWrapper& operator=(FuncWrapper const& o) = default;

		FuncWrapper(FuncWrapper&& o) noexcept
			: p(std::move(o.p)) {
		}

		inline FuncWrapper& operator=(FuncWrapper&& o) noexcept {
			std::swap(p, o.p);
			return *this;
		}

		FuncWrapper(lua_State* const& L, int const& idx) {
			if (!lua_isfunction(L, idx)) Error(L, "args[", std::to_string(idx), "] is not a lua function");
			CheckStack(L, 1);
			xx::MakeTo(p);
#ifdef USE_NEW_THREAD_HOLD_FUNC
			p->first = lua_newthread(L);								// ..., func, ..., L2
			p->second = luaL_ref(L, LUA_REGISTRYINDEX);					// ..., func, ...
			lua_pushvalue(L, idx);										// ..., func, ..., func
			lua_xmove(L, p->first, 1);									// ..., func, ...            L2: func
#else
			p->first = L;
			lua_pushvalue(L, idx);                                      // ..., func, ..., func
			p->second = luaL_ref(L, LUA_REGISTRYINDEX);                 // ..., func, ...
#endif
		}

		// 如果 p 引用计数唯一, 则反注册
		~FuncWrapper() {
			if (p.use_count() != 1) return;
			luaL_unref(p->first, LUA_REGISTRYINDEX, p->second);
		}
	};


	// 适配 std::function
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<xx::IsFunction_v<T>>> {
		using U = FunctionType_t<T>;

		static inline int Push(lua_State* const& L, T&& in) {
			PushUserdata(L, std::forward<T>(in));                       // ..., ud
			CheckStack(L, 1);
			lua_pushcclosure(L, [](auto L) {                            // ..., cc
				auto&& f = *(T*)lua_touserdata(L, lua_upvalueindex(1));
				FuncA_t<U> tuple;
				::xx::Lua::To(L, 1, tuple);
				int rtv = 0;
				std::apply([&](auto &... args) {
					if constexpr (std::is_void_v<FuncR_t<U>>) {
						f(args...);
					}
					else {
						rtv = xx::Lua::Push(L, f(args...));
					}
					}, tuple);
				return rtv;
				}, 1);
			return 1;
		}

		static inline void To(lua_State* const& L, int const& idx, T& out) {
			out = [fw = FuncWrapper(L, idx)](auto... args) {
				auto&& L = fw.p->first;									// func
#ifdef USE_NEW_THREAD_HOLD_FUNC
				lua_pushvalue(L, 1);									// func, func
				auto num = ::xx::Lua::Push(L, args...);                 // func, func, args...
				lua_call(L, num, LUA_MULTRET);                          // func, rtv...?
				if constexpr (!std::is_void_v<FuncR_t<U>>) {
					FuncR_t<FunctionType_t<T>> rtv;
					xx::Lua::To(L, 2, rtv);
					lua_settop(L, 1);                                   // func
					return rtv;
				}
				else {
					lua_settop(L, 1);                                   // func
				}
#else
				auto top = lua_gettop(L);
				CheckStack(L, 1);
				lua_rawgeti(L, LUA_REGISTRYINDEX, fw.p->second);        // ..., func
				auto num = ::xx::Lua::Push(L, args...);                 // ..., func, args...
				lua_call(L, num, LUA_MULTRET);                          // ..., rtv...?
				if constexpr (!std::is_void_v<FuncR_t<U>>) {
					FuncR_t<FunctionType_t<T>> rtv;
					xx::Lua::To(L, top + 1, rtv);
					lua_settop(L, top);                                 // ...
					return rtv;
				}
				else {
					lua_settop(L, top);                                 // ...
				}
#endif
			};
		}
	};

	// 适配 RefWrapper
	template<typename T>
	struct PushToFuncs<RefWrapper<T>, void> {
		static inline void To(lua_State* const& L, int const& idx, RefWrapper<T>& out) {
			PushToFuncs<T>::ToPtr(L, idx, out.p);
		}
	};

	// 适配 std::unordered_map<K, V>. 写入时体现为 table
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsUnorderedMap_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			lua_createtable(L, 0, (int)in.size());
			for (auto&& kv : in) {
				SetField(L, kv.first, kv.second);
			}
			return 1;
		}

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

	// 适配 std::vector<T>. 写入时体现为 table
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<IsVector_v<std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, T const& in) {
			auto siz = (int)in.size();
			lua_createtable(L, siz, 0);
			for (int i = 0; i < siz; ++i) {
				SetField(L, i + 1, in[i]);
			}
			return 1;
		}

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


	/******************************************************************************************************************/
	// Exec / PCall 返回值封装, 易于使用
	struct Result {
		int n = 0;
		std::string m;

		Result() = default;

		Result(Result const&) = default;

		Result& operator=(Result const&) = default;

		template<typename M>
		Result(int const& n, M&& m) : n(n), m(std::forward<M>(m)) {}

		Result(Result&& o) noexcept : n(std::exchange(o.n, 0)), m(std::move(o.m)) {}

		Result& operator=(Result&& o) noexcept {
			std::swap(n, o.n);
			std::swap(m, o.m);
			return *this;
		}

		inline operator bool() const {
			return n != 0;
		}
	};


	template<typename...Args>
	void AssertTop(lua_State* const& L, int const& n, Args &&...args) {
		auto top = lua_gettop(L);
		if (top != n) Error(L, "AssertTop( ", std::to_string(n), " ) failed! current top = ", std::to_string(top), std::forward<Args>(args)...);
	}

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

	inline Result PCallCore(lua_State* const& L, int const& top, int const& n) {
		Result rtv;
		if ((rtv.n = lua_pcall(L, n, LUA_MULTRET, 0))) {                // ... ( func? errmsg? )
			if (lua_isstring(L, -1)) {
				rtv.m = lua_tostring(L, -1);
			}
			else if (rtv.n == -1) {
				rtv.m = "cpp exception";
			}
			else {
				rtv.m = "lua_error forget arg?";
			}
			lua_settop(L, top);
		}
		return rtv;
	}

	// 安全调用函数( 函数最先压栈，然后是 up values ). 出错将还原 top
	// [-(nargs + 1), +(nresults|1), -]
	template<typename...Args>
	Result PCall(lua_State* const& L, Args &&...args) {
		auto top = lua_gettop(L);
		int n = 0;
		if constexpr (sizeof...(args) > 0) {
			n = Push(L, std::forward<Args>(args)...);
		}
		return PCallCore(L, top, n);
	}

	// 安全执行 lambda。出错将还原 top
	template<typename T>
	Result Try(lua_State* const& L, T&& func) {
		if (!CheckStack(L, 2)) return { -2, "lua_checkstack(L, 1) failed." };
		auto top = lua_gettop(L);
		lua_pushlightuserdata(L, &func);                                // ..., &func
		lua_pushcclosure(L, [](auto L) {                                // ..., cfunc
			auto&& f = (T*)lua_touserdata(L, lua_upvalueindex(1));
			(*f)();
			return 0;
			}, 1);
		return PCallCore(L, top, 0);
	}

	// 安全调用指定名称的全局函数( 会留下函数和返回值在栈中 )
	// [-(nargs + 1), +(nresults|1), -]
	template<typename T, typename...Args>
	Result PCallGlobalFunc(lua_State* const& L, T&& funcName, Args &&...args) {
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

	// 不安全调用指定名称的全局函数
	// [-(nargs + 1), +(nresults|1), e]
	template<typename T, typename...Args>
	void CallGlobalFunc(lua_State* const& L, T&& funcName, Args &&...args) {
		if constexpr (std::is_same_v<std::string, std::remove_const_t<std::remove_reference_t<T>>>) {
			lua_getglobal(L, funcName.c_str());
		}
		else {
			lua_getglobal(L, funcName);
		}
		Call(L, std::forward<Args>(args)...);
	}

	template<typename T, typename...Args>
	void CallFile(lua_State* const& L, T&& fileName, Args &&...args) {
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

	// 触发 lua 垃圾回收
	inline void GC(lua_State* const& L) {
		luaL_loadstring(L, "collectgarbage(\"collect\")");
		lua_call(L, 0, 0);
	}


	template<typename T, typename ENABLED>
	int PushToFuncs<T, ENABLED>::Push(lua_State* const& L, T&& in) {
		return PushUserdata(L, std::forward<T>(in));
	}

	template<typename T, typename ENABLED>
	template<typename U>
	void PushToFuncs<T, ENABLED>::ToPtr(lua_State* const& L, int const& idx, U*& out) {
		if (!lua_isuserdata(L, idx)) goto LabError;
		CheckStack(L, 2);
		lua_getmetatable(L, idx);                                           // ... tar(idx) ..., mt
		if (lua_isnil(L, -1)) goto LabError;
#if LUA_VERSION_NUM == 501
		lua_pushlightuserdata(L, (void*)MetaFuncs<U>::name.data());         // ... tar(idx) ..., mt, key
		lua_rawget(L, -2);                                                  // ... tar(idx) ..., mt, 1?
#else
		lua_rawgetp(L, -1, MetaFuncs<U>::name.data());                      // ... tar(idx) ..., mt, 1?
#endif
		if (lua_isnil(L, -1)) goto LabError;
		lua_pop(L, 2);                                                      // ... tar(idx) ...
		out = (U*)lua_touserdata(L, idx);
		return;
	LabError:
		Error(L, "error! args[", std::to_string(idx), "] is not ", MetaFuncs<U>::name, " ", std::to_string((size_t)MetaFuncs<U>::name.data()));
	}

	template<typename T, typename ENABLED>
	void PushToFuncs<T, ENABLED>::To(lua_State* const& L, int const& idx, T& out) {
		T* p = nullptr;
		ToPtr(L, idx, p);
		if constexpr (std::is_copy_assignable_v<T>) {
			out = *p;
		}
		else {
			out = std::move(*p);
		}
	}


	/******************************************************************************************************************/
	// 元表辅助填充类

	// C 传入 MetaFuncs 的 T
	template<typename C, typename B = void>
	struct Meta {
	protected:
		lua_State* const& L;
	public:
		explicit Meta(lua_State* const& L, std::string const& name) : L(L) {
			if constexpr (!std::is_void_v<B>) {
				MetaFuncs<B, void>::Fill(L);
			}
			SetField(L, (void*)name.data(), 1);
		}

		template<typename F>
		Meta& Func(char const* const& name, F const& f) {
			lua_pushstring(L, name);
			new(lua_newuserdata(L, sizeof(F))) F(f);                    // ..., ud
			lua_pushcclosure(L, [](auto L) {                            // ..., cc
				C* p = nullptr;
				PushToFuncs<C, void>::ToPtr(L, 1, p);
				auto&& self = ToPointer(*p);
				if (!self) Error(L, std::string("args[1] is nullptr?"));
				auto&& f = *(F*)lua_touserdata(L, lua_upvalueindex(1));
				FuncA_t<F> tuple;
				To(L, 2, tuple);
				int rtv = 0;
				std::apply([&](auto &... args) {
					if constexpr (std::is_void_v<FuncR_t<F>>) {
						((*self).*f)(std::move(args)...);
					}
					else {
						rtv = xx::Lua::Push(L, ((*self).*f)(std::move(args)...));
					}
					}, tuple);
				return rtv;
				}, 1);
			lua_rawset(L, -3);
			return *this;
		}

		template<typename P>
		Meta& Prop(char const* const& getName, P const& o) {
			SetField(L, (char*)getName, [o](C& self) {
				return (*ToPointer(self)).*o;
				});
			return *this;
		}

		template<typename P>
		Meta& Prop(char const* const& getName, char const* const& setName, P const& o) {
			if (getName) {
				Prop<P>(getName, o);
			}
			if (setName) {
				SetField(L, (char*)setName, [o](C& self, MemberPointerR_t<P> const& v) {
					(*ToPointer(self)).*o = v;
					});
			}
			return *this;
		}

		// lambda 第一个参数为 C& 类型，接受 self 传入
		template<typename F>
		Meta& Lambda(char const* const& name, F&& f) {
			lua_pushstring(L, name);
			Push(L, std::forward<F>(f));
			lua_rawset(L, -3);
			return *this;
		}
	};


	template<typename T, typename ENABLED>
	void MetaFuncs<T, ENABLED>::Fill(lua_State* const& L) {
		if constexpr (!IsLambda_v<T>) {
			Meta<T>(L, name);
		}
	}


	enum class LuaTypes : uint8_t {
		NilType, True, False, Integer, Double, String, Table, TableEnd      // todo: 将常见 int double 的 0 值  string 的 0 长 也提为类型
	};

	// L 栈顶的数据写入 d   (不支持 table 循环引用, 注意 lua5.1 不支持 int64, 整数需在 int32 范围内)
	inline void WriteTo(lua_State* const& in, xx::Data& d) {
		switch (auto t = lua_type(in, -1)) {
		case LUA_TNIL:
			d.WriteFixed(LuaTypes::NilType);
			return;
		case LUA_TBOOLEAN:
			d.WriteFixed(lua_toboolean(in, -1) ? LuaTypes::True : LuaTypes::False);
			return;
		case LUA_TNUMBER: {
#if LUA_VERSION_NUM == 501
			auto n = (double)lua_tonumber(in, -1);
			auto i = (int64_t)n;
			if ((double)i == n) {
				d.WriteFixed(LuaTypes::Integer);
				d.WriteVarInteger(i);
			}
			else {
				d.WriteFixed(LuaTypes::Double);
				d.WriteFixed(n);
			}
#else
			if (lua_isinteger(in, -1)) {
				d.WriteFixed(LuaTypes::Integer);
				d.WriteVarInteger((int64_t)lua_tointeger(in, -1));
			}
			else {
				d.WriteFixed(LuaTypes::Double);
				d.WriteFixed((double)lua_tonumber(in, -1));
			}
#endif
			return;
		}
		case LUA_TSTRING: {
			size_t len;
			auto ptr = lua_tolstring(in, -1, &len);
			d.WriteFixed(LuaTypes::String);
			d.WriteVarInteger(len);
			d.WriteBuf(ptr, len);
			return;
		}
		case LUA_TTABLE: {
			d.WriteFixed(LuaTypes::Table);
			int idx = lua_gettop(in);                                   // 存 idx 备用
			Lua::CheckStack(in, 1);
			lua_pushnil(in);                                            //                      ... t, nil
			while (lua_next(in, idx) != 0) {                            //                      ... t, k, v
				// 先检查下 k, v 是否为不可序列化类型. 如果是就 next
				t = lua_type(in, -1);
				if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA || t == LUA_TTHREAD) {
					lua_pop(in, 1);                                     //                      ... t, k
					continue;
				}
				t = lua_type(in, -2);
				if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TUSERDATA || t == LUA_TTHREAD) {
					lua_pop(in, 1);                                     //                      ... t, k
					continue;
				}
				WriteTo(in, d);                                         // 先写 v
				lua_pop(in, 1);                                         //                      ... t, k
				WriteTo(in, d);                                         // 再写 k
			}
			d.WriteFixed(LuaTypes::TableEnd);
			return;
		}
		default:
			d.WriteFixed(LuaTypes::NilType);
			return;
		}
	}

	// 从 d 读出一个 lua value 压入 L 栈顶
	// 如果读失败, 可能会有残留数据已经压入，外界需自己做 lua state 的 cleanup
	inline int ReadFrom(lua_State* const& out, xx::Data& d) {
		LuaTypes lt;
		if (int r = d.ReadFixed(lt)) return r;
		switch (lt) {
		case LuaTypes::NilType:
			Lua::CheckStack(out, 1);
			lua_pushnil(out);
			return 0;
		case LuaTypes::True:
			Lua::CheckStack(out, 1);
			lua_pushboolean(out, 1);
			return 0;
		case LuaTypes::False:
			Lua::CheckStack(out, 1);
			lua_pushboolean(out, 0);
			return 0;
		case LuaTypes::Integer: {
			int64_t i;
			if (int r = d.ReadVarInteger(i)) return r;
			Lua::CheckStack(out, 1);
			lua_pushinteger(out, (lua_Integer)i);
			return 0;
		}
		case LuaTypes::Double: {
			lua_Number v;
			if (int r = d.ReadFixed(v)) return r;
			Lua::CheckStack(out, 1);
			lua_pushnumber(out, v);
			return 0;
		}
		case LuaTypes::String: {
			size_t len;
			if (int r = d.ReadVarInteger(len)) return r;
			if (d.offset + len >= d.len) return __LINE__;
			Lua::CheckStack(out, 1);
			lua_pushlstring(out, (char*)d.buf + d.offset, len);
			d.offset += len;
			return 0;
		}
		case LuaTypes::Table: {
			Lua::CheckStack(out, 4);
			lua_newtable(out);                                          // ... t
			while (d.offset < d.len) {
				if (d.buf[d.offset] == (char)LuaTypes::TableEnd) {
					++d.offset;
					return 0;
				}
				if (int r = ReadFrom(out, d)) return r;                 // ... t, v
				if (int r = ReadFrom(out, d)) return r;                 // ... t, v, k
				lua_pushvalue(out, -2);                                 // ... t, v, k, v
				lua_rawset(out, -4);                                    // ... t, v
				lua_pop(out, 1);                                        // ... t
			}
		}
		default:
			return __LINE__;
		}
	}

	/******************************************************************************************************************/
	// Lua State 简单封装, 可直接当指针用, 离开范围自动 close
	struct State {
		lua_State* L = nullptr;

		inline operator lua_State* () {
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

		State(State&& o) noexcept : L(o.L) {
			o.L = nullptr;
		}

		State& operator=(State&& o) noexcept {
			std::swap(L, o.L);
			return *this;
		}
	};
}
