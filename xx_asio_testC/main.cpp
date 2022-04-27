// lobby + gateway + client -- client
#include "xx_asio_codes.h"
#include "pkg.h"

struct Client : xx::ServerCode<Client> {
	xx::ObjManager om;
	void Run(asio::ip::address const& addr, uint16_t const& port);
};

struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer, true>, std::enable_shared_from_this<CPeer> {
	Client& client;
	CPeer(Client& client_)
		: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
		, PeerTimeoutCode(client_.ioc)
		, PeerRequestCode(client_.om)
		, client(client_)
	{}

	std::unordered_set<uint32_t> openServerIds;

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

	awaitable<int> WaitOpen(uint32_t serviceId, std::chrono::steady_clock::duration const& d) {
		for (int i = 0; i < (d / 100ms); ++i) {
			if (openServerIds.contains(serviceId)) co_return 0;
			co_await xx::Timeout(100ms);
		}
		co_return __LINE__;
	}
};

void Client::Run(asio::ip::address const& addr, uint16_t const& port) {
	co_spawn(ioc, [=]()->awaitable<void> {

		// 创建个 peer
		auto p = std::make_shared<CPeer>(*this);

	LabBegin:
		// 如果没连上，就反复的连
		while (p->stoped) {
			// 开始连接. 超时 5 秒
			if (auto r = co_await p->Connect(addr, port, 5s)) {
				om.CoutN("Connect r = ", r);
			}
		}

		// 连上之后，等 open 0 号服务 5 秒
		if (auto r = co_await p->WaitOpen(0, 5s)) {
			om.CoutN("WaitOpen r = ", r);
			goto LabEnd;
		}

		// 等到了 open，给 0 号服务 发 Ping，超时 15 秒
		if (auto o = co_await p->SendRequest<Ping>(0, 15s, xx::NowSteadyEpoch10m()); !o) {
			om.CoutN("SendRequest Ping timeout!");
			goto LabEnd;
		}
		else {
			switch (o.typeId()) {
			case xx::TypeId_v<Pong>: {
				auto secs = (xx::NowSteadyEpoch10m() - o.ReinterpretCast<Pong>()->ticks) / 1000000.;
				om.CoutN("SendRequest Ping receive Pong. delay = ", secs);
				break;
			}
			default:
				om.CoutN("SendRequest Ping receive unhandled pkg = ", o);
				om.KillRecursive(o);
			}
		}

		// todo: other logic here

	LabEnd:
		// 掐线
		p->Stop();
		goto LabBegin;	// todo: 退出逻辑

	}, detached);
	ioc.run();
}

int main() {
	Client c;
	std::cout << "lobby + gateway + client -- client running..." << std::endl;
	c.Run(asio::ip::address::from_string("47.108.64.104"), 20000);
	return 0;
}
