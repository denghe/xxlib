// test xx_asio_tcp_gateway_client_cpp.h 

#include <xx_asio_tcp_gateway_client.h>

struct Logic {
	xx::Asio::Tcp::Gateway::Client c;
	Logic() {
		co_spawn(c.ioc, [this]()->awaitable<void> {
			c.SetDomainPort("127.0.0.1", 12345);												// 填充域名和端口
		LabBegin:
			c.Reset();																			// 开始前先 reset + sleep 一把，避免各种协程未结束的问题
			co_await xx::Timeout(1s);
			if (auto r = co_await c.Dial()) {													// 域名解析并拨号. 失败就重来
				xx::CoutTN("Dial r = ", r);
				goto LabBegin;
			}

			// co_await c.WaitServerIds(0);

			//c.Send<SS_C2S::Enter>();															// 发 enter 并等待收到 enter result 15 秒
			//auto er = co_await c.WaitPopPackage<SS_S2C::EnterResult>(15s);
			//if (!er) goto LabBegin;																// 超时: 重来
			//c.om.CoutTN(er);																	// 等到了 enter result. 打印

			//auto sync = co_await c.WaitPopPackage<SS_S2C::Sync>(15s);							// 等 sync 15 秒
			//if (!sync) goto LabBegin;															// 超时: 重来
			//c.om.CoutTN(sync);																	// 等到了 sync. 打印

			//while (c) {																			// 等待 断开 并打印收到的东西
			//	if (auto o = c.TryPopPackage()) {
			//		c.om.CoutTN(o);
			//	}
			//	// todo: 机器人逻辑. 模拟操作

			//	co_await xx::Timeout(1ms);														// 省点 cpu
			//};

			goto LabBegin;																		// 重来
			}, detached);
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
		logic.c.Update();																		// 每帧来一发
		std::this_thread::sleep_for(1ms);														// 模拟游戏循环延迟
	} while (true);
	xx::CoutN("end.");
	return 0;
}
