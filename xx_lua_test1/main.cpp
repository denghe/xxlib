#include <iostream>
#include "xx_lua_bind.h"
#include "xx_lua_data.h"
#include "xx_string.h"
#include "xx_lua_uv_client.h"
#ifdef _WIN32
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "userenv.lib")
#endif

void Test1() {
	auto L = luaL_newstate();
	luaL_openlibs(L);
	xx::Lua::Data::Register(L);

	lua_pushlightuserdata(L, nullptr);
	lua_setglobal(L, "nullptr");
	lua_pushlightuserdata(L, nullptr);
	lua_setglobal(L, "null");
	lua_pushlightuserdata(L, nullptr);
	lua_setglobal(L, "NULL");

	luaL_dofile(L, "test3.lua");
}


void Test2() {
	xx::Lua::State L;

	xx::CoutN("test Push NilType");
	{
		xx::Lua::Push(L, xx::Lua::NilType{});
		lua_setglobal(L, "a");
		luaL_dostring(L, "print(a)");
	}

	xx::CoutN("test Push To true");
	{
		xx::Lua::Push(L, true);
		bool a;
		xx::Lua::To(L, -1, a);
		lua_pop(L, 1);
		xx::CoutN(a);
	}

	xx::CoutN("test Push To false 12");
	{
		auto top = lua_gettop(L);
		xx::Lua::Push(L, false, 12);
		bool a;
		int b;
		xx::Lua::To(L, top + 1, a, b);
		lua_settop(L, top);
		xx::CoutN(a, " ", b);
	}

	xx::CoutN("test Push To Data(copy)");
	{
		xx::Data d;
		d.Fill({ 1,2,3,4,5 });
		xx::Lua::Push(L, d);
		auto d2 = xx::Lua::To<xx::Data*>(L, -1);
		xx::CoutN((d == *d2), " ", (&d == d2), d, *d2);
		lua_pop(L, 1);
	}

	xx::CoutN("test Push To Data(move)");
	{
		xx::Data d;
		d.Fill({ 1,2,3,4,5 });
		xx::Lua::Push(L, std::move(d));
		auto d2 = xx::Lua::To<xx::Data*>(L, -1);
		xx::CoutN((d == *d2), " ", (&d == d2), d, *d2);
		lua_pop(L, 1);
	}

	xx::CoutN("test To Func");
	{
		luaL_dostring(L, "function add(a,b) return a+b end");
		auto f = xx::Lua::GetGlobalFunc(L, "add");
		xx::CoutN(f.Call<int>(1, 2));
		xx::CoutN(f.Call<int>(3, 4));
	}

	//	xx::CoutN("test Lambda");
	//	{
	//		xx::Lua::SetGlobal(L, "xxx", [](int const& a, int const& b) { return a + b; });
	//				luaL_dostring(L, R"===(
	//local add = xxx
	//local starttime = os.clock()
	//local r
	//for i = 1, 30000000 do
	//    r = xxx(1, i)
	//end
	//print(r)
	//print(os.clock() - starttime)
	//)===");
	//	}

	assert(lua_gettop(L) == 0);
}

void TestUv() {
	xx::Lua::State L;
	SetGlobalCClosure(L, "Nows", [](auto L)->int { return xx::Lua::Push(L, xx::NowEpochSeconds()); });
	SetGlobalCClosure(L, "NowSteadyEpochMS", [](auto L)->int { return xx::Lua::Push(L, xx::NowSteadyEpochMilliseconds()); });
	xx::Lua::UvClient::Register(L);
	xx::Lua::Data::Register(L);

	auto r = xx::Lua::Try(L, [&] {
		xx::Lua::DoFile(L, "test_uv.lua");
		auto cb = xx::Lua::GetGlobalFunc(L, "gUpdate");
		while (true) {
			cb.Call();
			Sleep(16);
		}
	});
	if (r) {
		xx::CoutN(r.m);
	}
}

#include "xx_lua_bind_samples.h"

int main() {
	//Test1();
	//Test2();
	TestUv();
	//TestLuaBind1();
	//TestLuaBind2();
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


//	xx::CoutN("test cost");
//	{
//		luaL_dostring(L, R"===(
//local add = function(a,b) return a+b end
//local starttime = os.clock()
//local r
//for i = 1, 30000000 do
//    r = add(1, i)
//end
//print(r)
//print(os.clock() - starttime)
//)===");
//	}

//xx::CoutN("test To Func");
//{
//	luaL_dostring(L, "function add(a,b) return a+b end");
//	CheckStack(L, 5);
//	lua_getglobal(L, "add");
//	auto secs = xx::NowEpochSeconds();
//	int r;
//	for (size_t i = 0; i < 30000000; i++) {
//		lua_pushvalue(L, -1);
//		lua_pushinteger(L, 1);
//		lua_pushinteger(L, i);
//		lua_call(L, 2, 1);
//		r = lua_tointeger(L, -1);
//		lua_pop(L, 1);
//	}
//	xx::CoutN(r);
//	xx::CoutN("secs = ", xx::NowEpochSeconds() - secs);
//	lua_pop(L, 1);
//}

//xx::CoutN("test To Func1");
//{
//	luaL_dostring(L, "function add(a,b) return a+b end");
//	auto secs = xx::NowEpochSeconds();
//	int r;
//	for (size_t i = 0; i < 30000000; i++) {
//		auto top = lua_gettop(L);
//		CheckStack(L, 5);
//		lua_getglobal(L, "add");
//		lua_pushinteger(L, 1);
//		lua_pushinteger(L, i);
//		lua_call(L, 2, 1);
//		r = lua_tointeger(L, -1);
//		lua_settop(L, top);
//	}
//	xx::CoutN(r);
//	xx::CoutN("secs = ", xx::NowEpochSeconds() - secs);
//}

//xx::CoutN("test To Func2");
//{
//	luaL_dostring(L, "function add(a,b) return a+b end");
//	xx::Lua::Func f;
//	xx::Lua::GetGlobal(L, "add", f);
//	auto secs = xx::NowEpochSeconds();
//	int r;
//	for (size_t i = 0; i < 30000000; i++) {
//		r = f.Call<int>(1, i);
//	}
//	xx::CoutN(r);
//	xx::CoutN("secs = ", xx::NowEpochSeconds() - secs);
//}
