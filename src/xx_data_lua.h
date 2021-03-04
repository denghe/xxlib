﻿#pragma once

#ifndef MAKE_LIB
extern "C" {
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#ifndef MAKE_LIB
}
#endif

#include "xx_data.h"

/*
将 Data 封到 lua. 执行 xx::DataLua::Register( L ) 之后脚本中可用
全局函数有：
	NewXxData()		创建并返回 xx::Data( userdata ), 其元表提供一系列操作函数映射

成员函数见下: funcs

lua_checkstack 看情况加
*/

namespace xx::DataLua {
	using D = xx::Data;
	inline auto key = "xxData";

	// 读取参数1 Data 的工具函数
	inline D* GetSelf(lua_State* const& L) {
		return (D*)lua_touserdata(L, 1);
	}

	// 读取数值参数的工具函数
	template<typename T = size_t>
	inline T GetNum(lua_State* const& L, int const& idx) {
		if constexpr (std::is_floating_point_v<T>) return (T)lua_tonumber(L, idx);
#if LUA_VERSION_NUM == 501
		// todo: 兼容 int64 的处理
		return (T)lua_tonumber(L, idx);
#else
		return (T)lua_tointeger(L, idx);
#endif
	}

	inline int __gc(lua_State* L) {
		auto d = GetSelf(L);
		d->~D();
		return 0;
	}

	inline int Ensure(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = GetSelf(L);
		auto siz = GetNum(L, 2);
		d->Reserve(d->len + siz);
		return 0;
	}

	inline int Reserve(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = GetSelf(L);
		auto cap = GetNum(L, 2);
		d->Reserve(cap);
		return 0;
	}

	inline int Resize(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = GetSelf(L);
		auto len = GetNum(L, 2);
		auto r = d->Resize(len);
		lua_pushinteger(L, r);
		return 1;
	}

	inline int Clear(lua_State* L) {
		auto top = lua_gettop(L);
		assert(top >= 1);
		auto d = GetSelf(L);
		if (top == 1) {
			d->Clear();
		}
		else {
			bool b = lua_toboolean(L, 2) != 0;
			d->Clear(b);
		}
		return 0;
	}

	inline int GetCap(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = GetSelf(L);
		lua_pushinteger(L, d->cap);
		return 1;
	}

	inline int GetLen(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = GetSelf(L);
		lua_pushinteger(L, d->len);
		return 1;
	}

	inline int SetLen(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = GetSelf(L);
		auto len = GetNum(L, 2);
		d->len = len;
		return 0;
	}

	inline int GetOffset(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = GetSelf(L);
		lua_pushinteger(L, d->offset);
		return 1;
	}

	inline int SetOffset(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = GetSelf(L);
		auto offset = GetNum(L, 2);
		d->offset = offset;
		return 0;
	}

	// 读参数并调用 d->WriteXxxx
	template<typename T, bool isFixed = true, bool isBE = false, bool isAt = false>
	inline int W(lua_State* L) {
		assert(lua_gettop(L) > 1);
		auto d = GetSelf(L);
		if constexpr (isFixed) {
			if constexpr (isAt) {
				auto idx = GetNum<size_t>(L, 2);
				auto v = GetNum<T>(L, 3);
				if constexpr (isBE) {
					d->WriteFixedBEAt(idx, v);
				}
				else {
					d->WriteFixedAt(idx, v);
				}
			}
			else {
				auto v = GetNum<T>(L, 2);
				if constexpr (isBE) {
					d->WriteFixedBE(v);
				}
				else {
					d->WriteFixed(v);
				}
			}
		}
		else {
			auto v = GetNum<T>(L, 2);
			d->WriteVarInteger(v);
		}
		return 0;
	}

	// Wj
	// Ws
	// Ws_at
	inline int Wvi(lua_State* L) { return W<ptrdiff_t, false>(L); }
	inline int Wvu(lua_State* L) { return W<size_t, false>(L); }

	inline int Wi8(lua_State* L) { return W<int8_t>(L); }
	inline int Wu8(lua_State* L) { return W<uint8_t>(L); }
	inline int Wi16(lua_State* L) { return W<int16_t>(L); }
	inline int Wu16(lua_State* L) { return W<uint16_t>(L); }
	inline int Wi32(lua_State* L) { return W<int32_t>(L); }
	inline int Wu32(lua_State* L) { return W<uint32_t>(L); }
	inline int Wi64(lua_State* L) { return W<int64_t>(L); }
	inline int Wu64(lua_State* L) { return W<uint64_t>(L); }
	inline int Wf(lua_State* L) { return W<float>(L); }
	inline int Wd(lua_State* L) { return W<double>(L); }

	inline int Wi16_be(lua_State* L) { return W<int16_t, true, true>(L); }
	inline int Wu16_be(lua_State* L) { return W<uint16_t, true, true>(L); }
	inline int Wi32_be(lua_State* L) { return W<int32_t, true, true>(L); }
	inline int Wu32_be(lua_State* L) { return W<uint32_t, true, true>(L); }
	inline int Wi64_be(lua_State* L) { return W<int64_t, true, true>(L); }
	inline int Wu64_be(lua_State* L) { return W<uint64_t, true, true>(L); }
	inline int Wf_be(lua_State* L) { return W<float, true, true>(L); }
	inline int Wd_be(lua_State* L) { return W<double, true, true>(L); }

	inline int Wi8_at(lua_State* L) { return W<int8_t, true, false, true>(L); }
	inline int Wu8_at(lua_State* L) { return W<uint8_t, true, false, true>(L); }
	inline int Wi16_at(lua_State* L) { return W<int16_t, true, false, true>(L); }
	inline int Wu16_at(lua_State* L) { return W<uint16_t, true, false, true>(L); }
	inline int Wi32_at(lua_State* L) { return W<int32_t, true, false, true>(L); }
	inline int Wu32_at(lua_State* L) { return W<uint32_t, true, false, true>(L); }
	inline int Wi64_at(lua_State* L) { return W<int64_t, true, false, true>(L); }
	inline int Wu64_at(lua_State* L) { return W<uint64_t, true, false, true>(L); }
	inline int Wf_at(lua_State* L) { return W<float, true, false, true>(L); }
	inline int Wd_at(lua_State* L) { return W<double, true, false, true>(L); }

	inline int Wi16_be_at(lua_State* L) { return W<int16_t, true, true, true>(L); }
	inline int Wu16_be_at(lua_State* L) { return W<uint16_t, true, true, true>(L); }
	inline int Wi32_be_at(lua_State* L) { return W<int32_t, true, true, true>(L); }
	inline int Wu32_be_at(lua_State* L) { return W<uint32_t, true, true, true>(L); }
	inline int Wi64_be_at(lua_State* L) { return W<int64_t, true, true, true>(L); }
	inline int Wu64_be_at(lua_State* L) { return W<uint64_t, true, true, true>(L); }
	inline int Wf_be_at(lua_State* L) { return W<float, true, true, true>(L); }
	inline int Wd_be_at(lua_State* L) { return W<double, true, true, true>(L); }


	void AttachMT(lua_State* const& L);

	template<typename T>
	inline void Push(lua_State* const& L, T&& v) {
		if constexpr (std::is_floating_point_v<T>) {
			lua_pushnumber(L, (lua_Number)v);
		}
		else if constexpr (std::is_integral_v<T>) {
			lua_pushinteger(L, (lua_Integer)v);
		}
		else if constexpr (std::is_same_v<std::decay_t<T>, D>) {
			lua_newuserdata(L, sizeof(D));
			new (lua_touserdata(L, -1)) Data(std::forward(v));
			AttachMT(L);
		}
		// todo: more types here
		else throw - 1;
	}

	// 读参数并调用 d->ReadXxxx. 向 lua 压入 r, v ( 1 ~ 2 个参数 ). r == 0 则有 v
	template<typename T, bool isFixed = true, bool isBE = false, bool isAt = false>
	inline int R(lua_State* L) {
		assert(lua_gettop(L) >= 1);
		auto d = GetSelf(L);
		int r;
		T v;
		if constexpr (isFixed) {
			if constexpr (isAt) {
				auto idx = GetNum<size_t>(L, 2);
				if constexpr (isBE) {
					r = d->ReadFixedBEAt(idx, v);
				}
				else {
					r = d->ReadFixedAt(idx, v);
				}
			}
			else {
				if constexpr (isBE) {
					r = d->ReadFixedBE(v);
				}
				else {
					r = d->ReadFixed(v);
				}
			}
		}
		else {
			r = d->ReadVarInteger(v);
		}
		Push(L, r);
		if (r) return 1;
		Push(L, v);
		return 2;
	}
	// Rs
	// Rs_at
	inline int Rvi(lua_State* L) { return R<ptrdiff_t, false>(L); }
	inline int Rvu(lua_State* L) { return R<size_t, false>(L); }

	inline int Ri8(lua_State* L) { return R<int8_t>(L); }
	inline int Ru8(lua_State* L) { return R<uint8_t>(L); }
	inline int Ri16(lua_State* L) { return R<int16_t>(L); }
	inline int Ru16(lua_State* L) { return R<uint16_t>(L); }
	inline int Ri32(lua_State* L) { return R<int32_t>(L); }
	inline int Ru32(lua_State* L) { return R<uint32_t>(L); }
	inline int Ri64(lua_State* L) { return R<int64_t>(L); }
	inline int Ru64(lua_State* L) { return R<uint64_t>(L); }
	inline int Rf(lua_State* L) { return R<float>(L); }
	inline int Rd(lua_State* L) { return R<double>(L); }

	inline int Ri16_be(lua_State* L) { return R<int16_t, true, true>(L); }
	inline int Ru16_be(lua_State* L) { return R<uint16_t, true, true>(L); }
	inline int Ri32_be(lua_State* L) { return R<int32_t, true, true>(L); }
	inline int Ru32_be(lua_State* L) { return R<uint32_t, true, true>(L); }
	inline int Ri64_be(lua_State* L) { return R<int64_t, true, true>(L); }
	inline int Ru64_be(lua_State* L) { return R<uint64_t, true, true>(L); }
	inline int Rf_be(lua_State* L) { return R<float, true, true>(L); }
	inline int Rd_be(lua_State* L) { return R<double, true, true>(L); }

	inline int Ri8_at(lua_State* L) { return R<int8_t, true, false, true>(L); }
	inline int Ru8_at(lua_State* L) { return R<uint8_t, true, false, true>(L); }
	inline int Ri16_at(lua_State* L) { return R<int16_t, true, false, true>(L); }
	inline int Ru16_at(lua_State* L) { return R<uint16_t, true, false, true>(L); }
	inline int Ri32_at(lua_State* L) { return R<int32_t, true, false, true>(L); }
	inline int Ru32_at(lua_State* L) { return R<uint32_t, true, false, true>(L); }
	inline int Ri64_at(lua_State* L) { return R<int64_t, true, false, true>(L); }
	inline int Ru64_at(lua_State* L) { return R<uint64_t, true, false, true>(L); }
	inline int Rf_at(lua_State* L) { return R<float, true, false, true>(L); }
	inline int Rd_at(lua_State* L) { return R<double, true, false, true>(L); }

	inline int Ri16_be_at(lua_State* L) { return R<int16_t, true, true, true>(L); }
	inline int Ru16_be_at(lua_State* L) { return R<uint16_t, true, true, true>(L); }
	inline int Ri32_be_at(lua_State* L) { return R<int32_t, true, true, true>(L); }
	inline int Ru32_be_at(lua_State* L) { return R<uint32_t, true, true, true>(L); }
	inline int Ri64_be_at(lua_State* L) { return R<int64_t, true, true, true>(L); }
	inline int Ru64_be_at(lua_State* L) { return R<uint64_t, true, true, true>(L); }
	inline int Rf_be_at(lua_State* L) { return R<float, true, true, true>(L); }
	inline int Rd_be_at(lua_State* L) { return R<double, true, true, true>(L); }


	inline luaL_Reg funcs[] = {
		{ "__gc", __gc },

		// todo: ==, clone fill reset 啥的

		{ "Ensure", Ensure },
		{ "Reserve", Reserve },
		{ "Resize", Resize },
		{ "Clear", Clear },
		{ "GetCap", GetCap },
		{ "GetLen", GetLen },
		{ "SetLen", SetLen },
		{ "GetOffset", GetOffset },
		{ "SetOffset", SetOffset },
		//{ "__tostring", __tostring },

		//{ "Wj", Wj },
		//{ "Ws", Ws },
		//{ "Ws_at", Ws_at },

		{ "Wvi", Wvi },
		{ "Wvu", Wvu },

		{ "Wi8", Wi8 },
		{ "Wu8", Wu8 },
		{ "Wi16", Wi16 },
		{ "Wu16", Wu16 },
		{ "Wi32", Wi32 },
		{ "Wu32", Wu32 },
		{ "Wi64", Wi64 },
		{ "Wu64", Wu64 },
		{ "Wf", Wf },
		{ "Wd", Wd },

		{ "Wi16_be", Wi16_be },
		{ "Wu16_be", Wu16_be },
		{ "Wi32_be", Wi32_be },
		{ "Wu32_be", Wu32_be },
		{ "Wi64_be", Wi64_be },
		{ "Wu64_be", Wu64_be },
		{ "Wf_be", Wf_be },
		{ "Wd_be", Wd_be },

		{ "Wi8_at", Wi8_at },
		{ "Wu8_at", Wu8_at },
		{ "Wi16_at", Wi16_at },
		{ "Wu16_at", Wu16_at },
		{ "Wi32_at", Wi32_at },
		{ "Wu32_at", Wu32_at },
		{ "Wi64_at", Wi64_at },
		{ "Wu64_at", Wu64_at },
		{ "Wf_at", Wf_at },
		{ "Wd_at", Wd_at },

		{ "Wi16_be_at", Wi16_be_at },
		{ "Wu16_be_at", Wu16_be_at },
		{ "Wi32_be_at", Wi32_be_at },
		{ "Wu32_be_at", Wu32_be_at },
		{ "Wi64_be_at", Wi64_be_at },
		{ "Wu64_be_at", Wu64_be_at },
		{ "Wf_be_at", Wf_be_at },
		{ "Wd_be_at", Wd_be_at },


		//{ "Rs", Rs },
		//{ "Rs_at", Rs_at },

		//{ "Rvi", Rvi },
		//{ "Rvu", Rvu },

		{ "Ri8", Ri8 },
		{ "Ru8", Ru8 },
		{ "Ri16", Ri16 },
		{ "Ru16", Ru16 },
		{ "Ri32", Ri32 },
		{ "Ru32", Ru32 },
		{ "Ri64", Ri64 },
		{ "Ru64", Ru64 },
		{ "Rf", Rf },
		{ "Rd", Rd },

		{ "Ri16_be", Ri16_be },
		{ "Ru16_be", Ru16_be },
		{ "Ri32_be", Ri32_be },
		{ "Ru32_be", Ru32_be },
		{ "Ri64_be", Ri64_be },
		{ "Ru64_be", Ru64_be },
		{ "Rf_be", Rf_be },
		{ "Rd_be", Rd_be },

		{ "Ri8_at", Ri8_at },
		{ "Ru8_at", Ru8_at },
		{ "Ri16_at", Ri16_at },
		{ "Ru16_at", Ru16_at },
		{ "Ri32_at", Ri32_at },
		{ "Ru32_at", Ru32_at },
		{ "Ri64_at", Ri64_at },
		{ "Ru64_at", Ru64_at },
		{ "Rf_at", Rf_at },
		{ "Rd_at", Rd_at },

		{ "Ri16_be_at", Ri16_be_at },
		{ "Ru16_be_at", Ru16_be_at },
		{ "Ri32_be_at", Ri32_be_at },
		{ "Ru32_be_at", Ru32_be_at },
		{ "Ri64_be_at", Ri64_be_at },
		{ "Ru64_be_at", Ru64_be_at },
		{ "Rf_be_at", Rf_be_at },
		{ "Rd_be_at", Rd_be_at },

		{ nullptr, nullptr }
	};

	// 创建 mt 到栈顶
	inline void CreateMT(lua_State* const& L) {
		lua_createtable(L, 0, _countof(funcs));			// mt

		luaL_setfuncs(L, funcs, 0);						// mt
		lua_pushvalue(L, -1);							// mt, mt
		lua_setfield(L, -2, "__index");					// mt
		lua_pushvalue(L, -1);							// mt, mt
		lua_setfield(L, -2, "__metatable");				// mt
	}

	// 从注册表拿 mt 附加给 ud
	inline void AttachMT(lua_State* const& L) {				// ..., ud
		auto top = lua_gettop(L);
		lua_pushlightuserdata(L, (void*)key);				// ..., ud, key
		lua_rawget(L, LUA_REGISTRYINDEX);                   // ..., ud, mt
		assert(!lua_isnil(L, -1));
		lua_setmetatable(L, -2);							// ..., ud
		assert(top == lua_gettop(L));
	}



	// 注册到 lua 全局, 创建 xx::Data userdata
	inline int NewXxData(lua_State* L) {
		lua_newuserdata(L, sizeof(D));						// ..., ud
		new (lua_touserdata(L, -1)) Data();
		AttachMT(L);										// ..., ud
		return 1;
	}

	// 在 lua 中注册 全局的 Data 创建函数 和 注册表中的 metatable
	inline void Register(lua_State* const& L) {
		lua_pushcclosure(L, NewXxData, 0);					// ..., v
		lua_setglobal(L, "NewXxData");						// ...

		lua_pushlightuserdata(L, (void*)key);				// ..., key
		CreateMT(L);										// ..., key, mt
		lua_rawset(L, LUA_REGISTRYINDEX);					// ...
	}
}
