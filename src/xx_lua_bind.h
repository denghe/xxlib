#pragma once

// 与 lua 交互的代码应 Try 执行, 方能正确响应 luaL_error 或 C++ 异常
// luajit 有限支持 C++ 异常, 支持 中文变量名, 官方 lua 5.3/4 很多预编译库默认不支持, 必须强制以 C++ 方式自己编译
// 注意：To 系列 批量操作时，不支持负数 idx ( 递归模板里 + 1 就不对了 )
// 注意：这里使用 LUA_VERSION_NUM == 501 来检测是否为 luajit

#include "xx_lua.h"

namespace xx::Lua {

	/****************************************************************************************/
	/****************************************************************************************/
	// lua 向 c++ 注册的回调函数的封装, 将函数以 luaL_ref 方式放入注册表
	// 通常被 lambda 捕获携带, 析构时同步删除 lua 端函数。 需确保比 L 早死, 否则 L 就野了
	struct Func {
		Func() = default;
		Func(Func const& o) = default;
		Func& operator=(Func const& o) = default;
		Func(Func&& o) = default;
		Func& operator=(Func&& o) = default;

		// 存储 L 和 ref 值
		Shared<std::pair<lua_State*, int>> p;

		Func(lua_State* const& L, int const& idx) {
			Reset(L, idx);
		}
		~Func() {
			Reset();
		}

		// 从指定位置读出 func 弄到注册表并存储返回的 ref 值
		void Reset(lua_State* const& L, int const& idx) {
			if (!lua_isfunction(L, idx)) Error(L, "args[", std::to_string(idx), "] is not a lua function");
			Reset();
			CheckStack(L, 1);
			p.Emplace();
			p->first = L;
			lua_pushvalue(L, idx);                                      // ..., func, ..., func
			p->second = luaL_ref(L, LUA_REGISTRYINDEX);                 // ..., func, ...
		}

		// 如果 p 引用计数唯一, 则反注册
		void Reset() {
			if (p.useCount() == 1) {
				luaL_unref(p->first, LUA_REGISTRYINDEX, p->second);
			}
			p.Reset();
		}

		// 执行并返回
		template<typename T, typename...Args>
		T Call(Args&&...args) const {
			assert(p);
			auto& L = p->first;
			auto top = lua_gettop(L);
			CheckStack(L, 1);
			lua_rawgeti(L, LUA_REGISTRYINDEX, p->second);				// ..., func
			auto n = Push(L, args...);									// ..., func, args...
			lua_call(L, n, LUA_MULTRET);								// ..., rtv...?
			if constexpr (!std::is_void_v<T>) {
				T rtv;
				To(L, top + 1, rtv);
				lua_settop(L, top);										// ...
				return rtv;
			}
			else {
				lua_settop(L, top);										// ...
			}
		}

		// 判断是否有值
		operator bool() const {
			return (bool)p;
		}
	};

	// 适配 Func
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<Func, std::decay_t<T>>>> {
		static inline int Push(lua_State* const& L, Func const& f) {
			CheckStack(L, 1);
			if (f) {
				assert(f.p->first == L);
				lua_rawgeti(L, LUA_REGISTRYINDEX, f.p->second);
			}
			else {
				lua_pushnil(L);
			}
			return 1;
		}
	};

	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_same_v<Func, std::decay_t<T>>>> {
		static inline void To(lua_State* const& L, int const& idx, T& out) {
			out.Reset(L, idx);
		}
	};

	template<typename K, typename V>
	inline void GetGlobalFunc(lua_State* const& L, K const& k) {
		Func f;
		GetGlobal(L, k, f);
		return f;
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
	// 常用交互函数封装

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
	struct PushFuncs<T, std::enable_if_t<xx::IsLambda_v<std::decay_t<T>>>> {
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
						rtv = Push(L, (*f)(args...));
					}
					}, tuple);
				return rtv;
				}, 1);
			return 1;
		}
	};

	// 适配 RefWrapper
	template<typename T>
	struct ToFuncs<RefWrapper<T>, void> {
		static inline void To(lua_State* const& L, int const& idx, RefWrapper<T>& out) {
			PushToFuncs<T>::ToPtr(L, idx, out.p);
		}
	};


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
						rtv = Push(L, ((*self).*f)(std::move(args)...));
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
}
