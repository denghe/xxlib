/*
partial code
before include, need #define RefWeakTableRefId ............(L)
example:

using MyLuaState = decltype(GameLooper::L);
#define RefWeakTableRefId(L) MyLuaState::Extra(L)
*/
#include "xx_includes.h"    // xx_assert

namespace xx::Lua {

	inline void MakeUserdataWeakTable(lua_State* L) {
		// create weak table for userdata cache
		xx::Lua::AssertTop(L, 0);
		lua_createtable(L, 0, 1000);											// ... t
		lua_createtable(L, 0, 1);												// ... t, mt
		xx::Lua::SetField(L, "__mode", "v");									// ... t, mt { __mode = v }
		lua_setmetatable(L, -2);												// ... t
		RefWeakTableRefId(L) = luaL_ref(L, LUA_REGISTRYINDEX);					// ...
		xx::Lua::AssertTop(L, 0);
	}

	// wt push to stack
	inline void GetUserdataWeakTable(lua_State* L) {
		xx_assert(RefWeakTableRefId(L));
		CheckStack(L, 4);
		lua_rawgeti(L, LUA_REGISTRYINDEX, RefWeakTableRefId(L));// ..., wt
	}

	// store. value at top stack
	inline void StoreUserdataToWeakTable(lua_State* L, void* key) {
		GetUserdataWeakTable(L);												// ..., v, wt
		lua_pushvalue(L, -2);													// ..., v, wt, v
		lua_rawsetp(L, -2, key);												// ..., v, wt
		lua_pop(L, 1);															// ..., v
	}

	// found: push to stack & return true
	inline bool TryGetUserdataFromWeakTable(lua_State* L, void* key) {
		GetUserdataWeakTable(L);												// ..., wt
		auto typeId = lua_rawgetp(L, -1, key);									// ..., wt, v?
		if (typeId == LUA_TNIL) {
			lua_pop(L, 2);														// ...
			return false;
		}
		lua_replace(L, -2);														// ..., v
		return true;
	}

	inline void RemoveUserdataFromWeakTable(lua_State* L, void* key) {
		GetUserdataWeakTable(L);												// ..., wt
		lua_pushnil(L);															// ..., wt, nil
		lua_rawsetp(L, -2, key);												// ..., wt
		lua_pop(L, 1);															// ...
	}

	// match for Shared<T> luaL_setfuncs
	template<typename T, typename ENABLED = void>
	struct SharedTFuncs {
		inline static luaL_Reg funcs[] = {
			// ...
			{nullptr, nullptr}
		};
	};

	// meta
	template<typename T>
	struct MetaFuncs<T, std::enable_if_t<xx::IsShared_v<T>>> {
		using U = std::decay_t<T>;
		inline static std::string name = TypeName<U>();
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			luaL_setfuncs(L, SharedTFuncs<U>::funcs, 0);
			xx::Lua::SetFieldCClosure(L, "__gc", [](lua_State* L)->int {
				auto& p = *To<U*>(L);
				RemoveUserdataFromWeakTable(L, p.pointer);
				p.~U();
				return 0;
				});
		}
	};

	// push instance( move or copy )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<xx::IsShared_v<T>>> {
		using U = std::decay_t<T>;
		static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			if (!in) {
				lua_pushnil(L);
			} else if (!TryGetUserdataFromWeakTable(L, in.pointer)) {
				auto bak = in.pointer;
				PushUserdata<U, true>(L, std::forward<T>(in));
				StoreUserdataToWeakTable(L, bak);
			}
			return 1;
		}
	};

	// to pointer( for visit )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& xx::IsShared_v<std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>>> {
		using U = std::decay_t<std::remove_pointer_t<std::decay_t<T>>>;
		static void To_(lua_State* const& L, int const& idx, T& out) {
			if (lua_isnil(L, idx)) {
				out = nullptr;
			} else {
				AssertType<U>(L, idx);
				out = (T)lua_touserdata(L, idx);
			}
		}
	};
}
