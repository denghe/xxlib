// test xx_lua_asio_tcp_gateway_client.h 
// 等价于 xx_asio_testC, 属于它的 lua 版

#include <xx_lua_asio_tcp_gateway_client.h>

struct Logic : asio::noncopyable {
	xx::Lua::State L;
	Logic() {
		xx::Lua::RegisterGlobalUtils(L);	// null  NowEpochMS()  NowEpoch10m()  Int64ToNumber   NumberToInt64  Int64ToString  StringToInt64
		xx::Lua::Data::Register(L);
		xx::Lua::Asio::Tcp::Gateway::Client::Register(L);
		if (auto r = xx::Lua::Try(L, [&] {
			xx::Lua::DoFile(L, "main.lua");
		})) {
			xx::CoutN("Logic() DoFile 'main.lua' catch error n = ", r.n, " m = ", r.m);
		}
	}
	void Update() {
		if (auto r = xx::Lua::Try(L, [&] {
			xx::Lua::CallGlobalFunc(L, "GlobalUpdate");
		})) {
			xx::CoutN("Update() CallGlobalFunc 'GlobalUpdate' catch error n = ", r.n, " m = ", r.m);
		}
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
