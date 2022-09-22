// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// client
#include "xx_asio_tcp_gateway_client.h"
#include "pkg.h"

template<typename T>
using Package = xx::Asio::Tcp::Gateway::Package<xx::Shared<T>>;

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") 
#endif

int main() {
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	xx::Asio::Tcp::Gateway::Client c;														// 创建客户端
	c.SetDomainPort("127.0.0.1", 54000);													// 初始化连接参数
	uint32_t constexpr lobbyServerId = 0;													// 定义 lobby server 的 id 号（入口服务，直接和 server 约定好的）
	c.SetCppServerId(lobbyServerId);														// 设置当前 lobby server 的处理代码为 cpp 端

	// 主线逻辑: 连接 gateway 并 处理所有 lobby 消息
	co_spawn(c.ioc, [&]()->awaitable<void> {
	LabBegin:
		c.Reset();																			// 开始前先 reset + sleep 一把，避免各种协程未结束的问题
		co_await xx::Timeout(500ms);
		c.om.CoutTN("LabBegin");

		if (auto r = co_await c.Dial()) {													// 域名解析并拨号. 失败就重来
			xx::CoutTN("Dial error. r = ", r);
			goto LabBegin;
		}

		if (!co_await c.WaitOpens(5s, lobbyServerId)) {										// 连上之后，等 open 0 号服务 5 秒。超时就重来
			c.om.CoutTN("WaitOpens error.");
			goto LabBegin;
		}

		int64_t playerId = -1;																// 待填充的 当前玩家 id
		if (auto o = co_await c.SendRequestTo<Client_Lobby::Login>(lobbyServerId, 5s, "a"sv, "1"sv)) {	// 向 lobby 发个 Login 等回应
			switch (o.GetTypeId()) {
			case xx::TypeId_v<Generic::Success>: {
				playerId = o.ReinterpretCast<Generic::Success>()->value;
				break;
			}
			default:
				c.om.CoutTN("SendRequest Login. username = 'a', password = '1', receive unhandled o = ", o);
				goto LabBegin;
			}
		} else goto LabBegin;

		xx::Shared<Generic::PlayerInfo> playerInfo;											// 待填充的 玩家信息
		std::string gamesIntro;																// 待填充的 大厅游戏列表啥的
		{
			Package<Lobby_Client::Scene> pkg;												// 待填充的 pop 容器
			if (co_await c.Pop(15s, pkg, lobbyServerId)) {									// 等一个来自 lobby 的包，类型是 Scene. 如果等到: 将数据移出来
				playerInfo = std::move(pkg.data->playerInfo);
				gamesIntro = std::move(pkg.data->gamesIntro);
			} else goto LabBegin;
		}

		c.om.CoutTN("playerInfo = ", playerInfo);
		c.om.CoutTN("lobby games intro = ", gamesIntro);

		// todo: enter game

		Package<xx::ObjBase> pkg;															// 继续处理各种 push / request 之 待填充 临时容器
	LabProcess:
		if (!c.TryPop(pkg, lobbyServerId)) {												// 试弹出一条属于 lobby 的消息. 如果没有就 睡一帧
			co_await xx::Timeout(0s);
			if (!c) goto LabBegin;															// 已断开?
			goto LabProcess;																// 继续取
		}
		switch (pkg.data.GetTypeId()) {
			// case ....
		default:
		{
			c.om.CoutTN("receive pkg serverId = ", pkg.serverId, ", serial = ", pkg.serial, ", data = ", pkg.data);
			c.om.KillRecursive(pkg.data);													// 防范递归引用泄露
		}
		}
		goto LabProcess;

	}, detached);

	// 并行逻辑: 等 lobby open 后不断发 ping
	co_spawn(c.ioc, [&]()->awaitable<void> {
	LabBegin:
		co_await xx::Timeout(3s);
		if (!co_await c.WaitOpens(1s, 0)) goto LabBegin;
		if (auto o = co_await c.SendRequestTo<Ping>(0, 5s, xx::NowSteadyEpoch10m()); o.GetTypeId() == xx::TypeId_v<Pong>) {
			c.om.CoutTN("delay ms = ", double(xx::NowSteadyEpoch10m() - o.ReinterpretCast<Pong>()->ticks) / 10000.0);
		} else {
			c.om.KillRecursive(o);
		}
		goto LabBegin;
	}, detached);

	// 模拟游戏每帧来一发
	std::cout << "client running..." << std::endl;
	while (true) {
		c.ioc.poll();
		std::this_thread::sleep_for(16ms);			// 模拟渲染 + wait vsync
	}
	return 0;
}
