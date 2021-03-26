#pragma once

// 与 lua 交互的代码应 Try 执行, 方能正确响应 luaL_error 或 C++ 异常
// luajit 有限支持 C++ 异常, 支持 中文变量名, 官方 lua 5.3/4 很多预编译库默认不支持, 必须强制以 C++ 方式自己编译
// 注意：To 系列 批量操作时，不支持负数 idx ( 递归模板里 + 1 就不对了 )
// 注意：这里使用 LUA_VERSION_NUM == 501 来检测是否为 luajit

#include "xx_lua.h"

namespace xx::Lua {

	/****************************************************************************************/
	/****************************************************************************************/
	// 进一步增加基础类型和工具函数



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
