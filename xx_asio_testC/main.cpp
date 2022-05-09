// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// client
#include "xx_asio_tcp_gateway_client_cpp.h"
#include "pkg.h"

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") 
#endif

int main() {
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	xx::Asio::Tcp::Gateway::Cpp::Client c;
	co_spawn(c.ioc, [&]()->awaitable<void> {
		c.SetDomainPort("127.0.0.1", 54000);												// 初始化连接参数
		c.AddCppServerIds(0);																// 设置当前 0 号 server 的处理代码为 cpp 端

	LabBegin:
		c.om.CoutTN("LabBegin");

		c.Reset();																			// 开始前先 reset + sleep 一把，避免各种协程未结束的问题
		co_await xx::Timeout(500ms);

		if (auto r = co_await c.Dial()) {													// 域名解析并拨号. 失败就重来
			xx::CoutTN("Dial error. r = ", r);
			goto LabBegin;
		}
		c.om.CoutTN(__LINE__, " c = ", !!c);

		if (!co_await c.WaitOpens(5s, 0)) {													// 连上之后，等 open 0 号服务 5 秒。超时就重来
			c.om.CoutTN("WaitOpens error.");
			goto LabBegin;
		}
		c.om.CoutTN(__LINE__, " c = ", !!c);

		if (auto o = co_await c.SendRequestTo<Ping>(0, 15s, xx::NowSteadyEpoch10m()); o.typeId() == xx::TypeId_v<Pong>) { // 给 0 号服务 发 Ping，等 回包, 超时 15 秒
			c.om.CoutTN("delay ms = ", double(xx::NowSteadyEpoch10m() - o.ReinterpretCast<Pong>()->ticks) / 10000.0);
		} else goto LabBegin;

		if (auto o = co_await c.SendRequestTo<Client_Lobby::Login>(0, 15s, "a"sv, "1"sv)) {	// 继续发个包等回应
			c.om.CoutTN("SendRequest Login a 1, o = "sv, o);								// 输出返回值
		} else goto LabBegin;

		goto LabBegin;
	}, detached);

	// 模拟游戏每帧来一发
	std::cout << "client running..." << std::endl;
	while (true) {
		c.ioc.poll_one();
		//std::cout << ".";	
		std::this_thread::sleep_for(8ms);			// 模拟逻辑损耗
		c.ioc.poll_one();
		std::this_thread::sleep_for(8ms);			// 模拟渲染 + wait vsync
	}
	return 0;
}
