// lobby + gateway + client -- client
#include "xx_asio_codes.h"
#include "pkg.h"

struct Client : xx::IOCCode<Client> {
	xx::ObjManager om;
	void Run(asio::ip::address addr, uint16_t port);
};

struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer, true>, std::enable_shared_from_this<CPeer> {
	Client& client;
	std::unordered_set<uint32_t> openServerIds;	// 白名单

	CPeer(Client& client_)
		: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
		, PeerTimeoutCode(client_.ioc)
		, PeerRequestCode(client_.om)
		, client(client_)
	{}
	int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
		if (target == 0xFFFFFFFF) {
			std::string_view cmd;
			if (dr.Read(cmd)) return -__LINE__;	// error
			uint32_t serviceId;
			if (cmd == "open"sv) {
				if (dr.Read(serviceId)) return -__LINE__;	// read error
				openServerIds.insert(serviceId);
			}
			else if (cmd == "close"sv) {
				if (dr.Read(serviceId)) return __LINE__;	// read error
				openServerIds.erase(serviceId);
			}
			else return -__LINE__;	// cmd error: unknown cmd received
			return 0;	// handled: success
		}
		return 1;	// passthrough
	}

	awaitable<int> WaitOpen(uint32_t serviceId, std::chrono::steady_clock::duration d) {
		for (int i = 0; i < (d / 100ms); ++i) {
			if (stoped) co_return __LINE__;
			if (openServerIds.contains(serviceId)) co_return 0;
			co_await xx::Timeout(100ms);
		}
		co_return __LINE__;
	}
};

void Client::Run(asio::ip::address addr, uint16_t port) {
	co_spawn(ioc, [this, addr, port]()->awaitable<void> {

		// 创建个 peer
		auto p = std::make_shared<CPeer>(*this);

	LabBegin:
		// 如果没连上，就反复的连
		while (p->stoped) {
			// 开始连接. 超时 5 秒
			if (auto r = co_await p->Connect(addr, port, 5s)) {
				om.CoutN("Connect error. r = ", r, ". retry...");
			}
		}

		// 连上之后，等 open 0 号服务 5 秒
		if (auto r = co_await p->WaitOpen(0, 5s)) {
			om.CoutN("WaitOpen error. r = ", r, ". reconnect...");
			goto LabEnd;
		}

		// 等到了 open，给 0 号服务 发 Ping，超时 15 秒
		om.CoutN("SendRequest Ping");
		if (auto o = co_await p->SendRequest<Ping>(0, 5s, xx::NowSteadyEpoch10m()); !o) {
			om.CoutN("timeout!");
			goto LabEnd;
		}
		else {
			switch (o.typeId()) {
			case xx::TypeId_v<Pong>: {
				auto ms = (xx::NowSteadyEpoch10m() - o.ReinterpretCast<Pong>()->ticks) / 1000.;
				om.CoutN("receive Pong. delay = ", ms);
				break;
			}
			default:
				om.CoutN("receive unhandled pkg = ", o);
				om.KillRecursive(o);
			}
		}

		// 等超时断开
		//while (p->Alive()) {
			co_await xx::Timeout(1s);	// todo: other logic here
			std::cout << ".";
			std::cout.flush();
		//}

	LabEnd:
		// 掐线 / clean up
		p->Stop();

		// 小睡一下，让别的协程处理下断线逻辑
		co_await xx::Timeout(2s);

		// 再来( 自动重连 )
		goto LabBegin;

	}, detached);
	ioc.run();
}

int main() {
	Client c;
	std::cout << "lobby + gateway + client -- client running..." << std::endl;
	c.Run(asio::ip::address::from_string("127.0.0.1"), 54322);
	return 0;
}
