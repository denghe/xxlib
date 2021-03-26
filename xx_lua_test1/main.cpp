#include "xx_data_lua.h"
#include <iostream>

void Test1() {
	auto L = luaL_newstate();
	luaL_openlibs(L);
	xx::DataLua::Register(L);

	lua_pushlightuserdata(L, nullptr);
	lua_setglobal(L, "nullptr");
    lua_pushlightuserdata(L, nullptr);
    lua_setglobal(L, "null");
	lua_pushlightuserdata(L, nullptr);
	lua_setglobal(L, "NULL");

	luaL_dofile(L, "test3.lua");

}

#include "xx_lua.h"

void Test2() {
	auto L = luaL_newstate();
	luaL_openlibs(L);

	{
		xx::Lua::Push(L, xx::Lua::NilType{});
		lua_setglobal(L, "a");
		luaL_dostring(L, "print(a)");
	}

	{
		xx::Lua::Push(L, true);
		lua_setglobal(L, "a");
		luaL_dostring(L, "print(a)");
	}

	{
		xx::Lua::Push(L, true);
		bool a;
		xx::Lua::To(L, -1, a);
		lua_pop(L, 1);
		std::cout << a << std::endl;
	}

	{
		auto top = lua_gettop(L);
		xx::Lua::Push(L, false);
		xx::Lua::Push(L, 12);
		bool a;
		int b;
		xx::Lua::To(L, top + 1, a, b);
		lua_settop(L, top);
		std::cout << a << " " << b << std::endl;
	}


}

int main() {
	//Test1();
	Test2();
	std::cout << "end." << std::endl;
	return 0;
}


// todo: close

//auto top = lua_gettop(L);
//int n;
//char const* m;
//if ((n = lua_pcall(L, 1, LUA_MULTRET, 0))) {                // ... ( func? errmsg? )
//	if (lua_isstring(L, -1)) {
//		m = lua_tostring(L, -1);
//	}
//	else if (n == -1) {
//		m = "cpp exception";
//	}
//	else {
//		m = "lua_error forget arg?";
//	}
//	lua_settop(L, top);
//}
//if (n) {
//	std::cout << "error occur: n = " << n << " m = " << m << std::endl;
//}
