// test xx_lua_asio_tcp_gateway_client.h 
// 等价于 xx_asio_testC, 属于它的 lua 版

#include <xx_lua_asio_tcp_gateway_client.h>

struct Logic : asio::noncopyable {
	xx::Lua::State L;
	Logic() {
		xx::Lua::Data::Register(L);
		xx::Lua::Asio::Tcp::Gateway::Client::Register(L);
		xx::Lua::SetGlobalCClosure(L, "NowSteadyEpochMS", [](auto L)->int { return xx::Lua::Push(L, (double)xx::NowSteadyEpochMilliseconds()); });
		xx::Lua::SetGlobal(L, "null", (void*)0);
		xx::Lua::DoFile(L, "lib.lua");
		xx::Lua::DoFile(L, "main.lua");
	}
	void Update() {
		xx::Lua::CallGlobalFunc(L, "GlobalUpdate");
	}
};

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") 
#endif

int main() {
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	Logic logic;
	do {
		logic.Update();																			// 每帧来一发
		std::this_thread::sleep_for(16ms);														// 模拟游戏循环延迟
	} while (true);
	xx::CoutN("end.");
	return 0;
}
