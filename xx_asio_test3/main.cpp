// simple tcp test client for xx_asio_test2

#include <xx_asio_tcp_client_cpp.h>
#include "ss.h"

struct Logic {
    xx::Asio::Tcp::Cpp::Client c;
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
			c.SendPush<SS_C2S::Enter>();														// 发 Enter 

			if (auto er = co_await c.PopPush<SS_S2C::EnterResult>(15s)) {						// 等 EnterResult 包 15 秒
				c.om.CoutTN(er);																// 打印
			} else goto LabBegin;

			if (auto sync = co_await c.PopPush<SS_S2C::Sync>(15s)) {							// 继续等 sync 包 15 秒
				c.om.CoutTN(sync);																// 打印
			} else goto LabBegin;

			while(c) {																			// 等待 断开 并打印收到的东西
				if (auto o = c.TryPopPush()) {
					c.om.CoutTN(o);
				}																				// todo: 机器人逻辑. 模拟操作

				co_await xx::Timeout(1ms);														// 省点 cpu
			};

			goto LabBegin;																		// 断开了: 重来
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
	} while(true);
	xx::CoutN("end.");
	return 0;
}
