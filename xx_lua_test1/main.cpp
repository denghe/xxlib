#include <iostream>
#include "xx_lua_bind.h"
#include "xx_lua_data.h"
#include "xx_string.h"

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

    xx::CoutN(lua_gettop(L));
    if (luaL_dofile(L, "test3.lua") != LUA_OK) {
        std::string s;
        xx::Lua::To(L, -1, s);
        xx::CoutN(s);
        lua_pop(L, 1);
    }
    xx::CoutN(lua_gettop(L));
}


void Test2() {
    xx::Lua::State L;
    //lua_State *L = lua_newstate( xxxxxmalloc , NULL);
    //luaL_openlibs(L);

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

    xx::CoutN("test Push To optional<string>");
    {
        auto top = lua_gettop(L);
        std::optional<std::string> ss;
        ss = "asdf";
        xx::Lua::Push(L, std::move(ss));
        ss.reset();
        xx::Lua::To(L, top + 1, ss);
        lua_settop(L, top);
        xx::CoutN(ss);
    }

    xx::CoutN("test Push To pair<optional<string>, int>");
    {
        auto top = lua_gettop(L);
        std::pair<std::optional<std::string>, int> ss;
        ss.first = "asdf";
        ss.second = 123;
        xx::Lua::Push(L, ss);
        ss = decltype(ss)();
        xx::Lua::To(L, top + 1, ss);
        lua_settop(L, top);
        xx::CoutN(ss);
    }

    xx::CoutN("test Push To vector<string>");
    {
        auto top = lua_gettop(L);
        std::vector<std::string> ss;
        ss.emplace_back("asdf");
        ss.emplace_back("qwer");
        xx::Lua::Push(L, ss);
        ss.clear();
        xx::Lua::To(L, top + 1, ss);
        lua_settop(L, top);
        xx::CoutN(ss);
    }

    xx::CoutN("test Push To map<string, int>");
    {
        auto top = lua_gettop(L);
        std::map<std::string, int> ss;
        ss["asdf"] = 123;
        ss["qwer"] = 234;
        xx::Lua::Push(L, ss);
        ss.clear();
        xx::Lua::To(L, top + 1, ss);
        lua_settop(L, top);
        xx::CoutN(ss);
    }

    xx::CoutN("test Push To map<string, vector<int>>");
    {
        auto top = lua_gettop(L);
        std::map<std::string, std::vector<int>> ss;
        ss["asdf"].push_back(1);
        ss["qwer"].push_back(2);
        ss["qwer"].push_back(3);
        xx::Lua::Push(L, ss);
        ss.clear();
        xx::Lua::To(L, top + 1, ss);
        lua_settop(L, top);
        xx::CoutN(ss);
    }

    xx::CoutN("test Push To Data(copy)");
    {
        xx::Data d;
        d.Fill({1, 2, 3, 4, 5});
        xx::Lua::Push(L, d);
        auto d2 = xx::Lua::To<xx::Data *>(L, -1);
        xx::CoutN((d == *d2), " ", (&d == d2), d, *d2);
        lua_pop(L, 1);
    }

    xx::CoutN("test Push To Data(move)");
    {
        xx::Data d;
        d.Fill({1, 2, 3, 4, 5});
        xx::Lua::Push(L, std::move(d));
        auto d2 = xx::Lua::To<xx::Data *>(L, -1);
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

//    xx::CoutN("test Lambda");
//    {
//        xx::Lua::SetGlobal(L, "xxx", [](int const &a, int const &b) { return a + b; });
//        luaL_dostring(L, R"===(
//local add = xxx
//local starttime = os.clock()
//local r
//for i = 1, 30000000 do
//    r = xxx(1, i)
//end
//print(r)
//print(os.clock() - starttime)
//    )===");
//    }

    assert(lua_gettop(L) == 0);
}


#include "xx_lua_uv_client.h"

#include "xx_lua_bind_samples.h"

//#include "luasocket.h"
//#include "mime.h"

void TestUv() {
    xx::Lua::State L;

//    lua_getglobal(L, "package");                    // ..., t
//    lua_pushstring(L, "preload");                   // ..., t, ""
//    lua_rawget(L, -2);                              // ..., t, t
//    lua_pushstring(L, "socket.core");               // ..., t, t, ""
//    lua_pushcclosure(L, luaopen_socket_core, 0);    // ..., t, t, "", f
//    lua_rawset(L, -3);                              // ..., t, t
//    lua_pushstring(L, "mime.core");                 // ..., t, t, ""
//    lua_pushcclosure(L, luaopen_mime_core, 0);      // ..., t, t, "", f
//    lua_rawset(L, -3);                              // ..., t, t
//    lua_pop(L, 2);                                  // ...,

    SetGlobalCClosure(L, "Nows", [](auto L) -> int { return xx::Lua::Push(L, xx::NowEpochSeconds()); });
    SetGlobalCClosure(L, "NowSteadyEpochMS", [](auto L) -> int { return xx::Lua::Push(L, xx::NowSteadyEpochMilliseconds()); });
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


#include "xx_ptr.h"
#include <optional>
#include <variant>

struct LuaValue : std::variant<ptrdiff_t, double, std::string, xx::Shared<std::unordered_map<LuaValue, LuaValue>>> {
    using BaseType = std::variant<ptrdiff_t, double, std::string, xx::Shared<std::unordered_map<LuaValue, LuaValue>>>;
    using BaseType::BaseType;
    std::string ToString() const;
};
using LuaTable = std::unordered_map<LuaValue, LuaValue>;
using LuaTable_s = xx::Shared<LuaTable>;
using NullableLuaValue = std::optional<LuaValue>;

namespace std {
    template<>
    struct hash<LuaValue> {
        size_t operator()(LuaValue const& v) const {
            switch (v.index()) {
                case 0: return (size_t)std::get<ptrdiff_t>(v);
                case 1: return hash<double>()(std::get<double>(v));
                case 2: return hash<std::string>()(std::get<std::string>(v));
                case 3: return (size_t)std::get<LuaTable_s>(v).pointer;
            }
            assert(false);
            return 0;
        }
    };
}

inline std::string LuaValue::ToString() const {
    switch (index()) {
        case 0: return std::to_string(std::get<ptrdiff_t>(*this));
        case 1: return std::to_string(std::get<double>(*this));
        case 2: return std::get<std::string>(*this);
        case 3: return std::to_string((size_t)std::get<xx::Shared<std::unordered_map<LuaValue, LuaValue>>>(*this).pointer);
    }
    return "";
}

void Test3() {
    LuaValue v = xx::Make<LuaTable>();
    auto& t = *std::get<LuaTable_s>(v);
    t.emplace((ptrdiff_t)1, "asdf");
    t.emplace(2.3, "qwer");
    for(auto& kv : t) {
        std::cout << kv.first.ToString() << " " << kv.second.ToString() << std::endl;
    }
}

#include "xx_lua_randoms.h"

void TestTableToData() {
    xx::Lua::State L;
    xx::Lua::Randoms::Register(L);
    xx::Lua::DoString(L, R"#(
rnd = NewXxRandom1(1234567890)
print(rnd)
print(rnd:NextDouble())
print(rnd:NextDouble())
print(rnd:NextDouble())
print(rnd:NextDouble())
)#");
    auto top = lua_gettop(L);
    lua_getglobal(L, "rnd");    // ..., rnd
    xx::Data d;
    xx::Lua::WriteTo(d, L);
    lua_pop(L, 1);              // ...
    assert(top == lua_gettop(L));
    assert(d.len = 7);          // 1(6) + 2(1) + 4(seed)
    xx::CoutN(d);

    if (int r = xx::Lua::ReadFrom(d, L)) {    // ..., rnd
        xx::CoutN("read error. r = ", r);
        return;
    }
    assert(d.offset == d.len);
    lua_setglobal(L, "rnd2");   // ...
    xx::Lua::DoString(L, R"#(
print(rnd2)
print(rnd2:NextDouble())
print(rnd2:NextDouble())
print(rnd2:NextDouble())
print(rnd2:NextDouble())
)#");
}

int main() {
    //Test1();
    Test2();
    //TestUv();
    //TestLuaBind1();
    //TestLuaBind2();
    //TestTableToData();
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
