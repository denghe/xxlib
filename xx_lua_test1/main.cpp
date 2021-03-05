#include "xx_data_lua.h"
#include <iostream>

void Test1() {
	auto L = luaL_newstate();
	luaL_openlibs(L);
	xx::DataLua::Register(L);

	luaL_dostring(L, R"=(
local d = NewXxData()
getmetatable(d).Dump = function(tar, prefix) print(prefix, tar:GetAll()) end

d:Dump("NewXxData")
d:Fill(1,2,3,4,5,6,7)
d:Dump("Fill     ")
local d2 = d:Copy()
d2:Dump("Copy     ")
print("Equals    ", d:Equals(d2))
d:Clear()
d:Dump("Clear    ")
print("Equals    ", d:Equals(d2))
d:Clear(true)
d:Dump("Clear(true)")
d:Ensure(5)
d:Dump("Ensure    ")
d:Reserve(20)
d:Dump("Reserve   ")
d:Fill(1,2)
d:Dump("Fill      ")
print("GetCap     ", d:GetCap())
print("GetLen     ", d:GetLen())
print("GetOffset  ", d:GetOffset())
d:SetLen(5)
d:Dump("SetLen    ")
d:SetOffset(3)
d:Dump("SetOffset  ")
d:Clear(true)
d:Dump("Clear(true)")
local bak = d:GetLen()
d:Wj(4)
d:Dump("Wj         ")
local s = "asdfqwe"
d:Ws(s)
print("Ws          ", d)
d:Wu32_at(bak, d:GetLen()-bak)
d:Dump("Wu32_at    ")
print("Wu32_at     ", d)
print("At          ", d:At(0))

d:Clear()
d:Wvu(#s)
d:Ws(s)
d:Dump("Wvu Ws    ")
print("Wvu Ws    ", d)
local r, slen = d:Rvu()
print("Rvu         ", r, slen)
local r, s2 = d:Rs(slen)
print("Rs          ", r, #s2, s2)

d:Clear()
d:Wf(1.23)
print("Wf        ", d)
d:Wd(1.23)
print("Wd        ", d)
d:Dump("Wd         ")
local r, f = d:Rf()
d:Dump("Rf         ")
print("Rf         ", r, f)
local r, dbl = d:Rd()
print("Rd         ", r, dbl)
d:Dump("Rd         ")
local r, dbl = d:Rd()
print("Rd         ", r, dbl)	-- line number, nil


)=");

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
}

void Test2() {
}

int main() {
	Test1();
	//Test2();

	std::cout << "end." << std::endl;
	return 0;
}
