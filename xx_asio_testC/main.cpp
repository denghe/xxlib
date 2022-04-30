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

	// 为复用，这里重置所有状态 & 变量 & 容器
	void Start_() {
		openServerIds.clear();
	}

	// 针对 target 部分为 0xFFFFFFFFu 的指令特殊处理。放行返回 1. 成功返回 0. 错误返回 负数
	int HandleTargetMessage(uint32_t const& target, xx::Data_r& dr) {
		if (target != 0xFFFFFFFFu) return 1;	// passthrough
		std::string_view cmd;
		if (dr.Read(cmd)) return -__LINE__;
		uint32_t serviceId;
		if (cmd == "open"sv) {
			if (dr.Read(serviceId)) return -__LINE__;
			openServerIds.insert(serviceId);
		}
		else if (cmd == "close"sv) {
			if (dr.Read(serviceId)) return -__LINE__;
			openServerIds.erase(serviceId);
		}
		else {
			xx::CoutTN("unknown cmd received: ", cmd);
			return -__LINE__;
		}
		return 0;	// success
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
		om.CoutTN("begin ...");
		// 如果没连上，就反复的连
		while (p->stoped) {
			// 开始连接. 超时 5 秒
			if (auto r = co_await p->Connect(addr, port, 5s)) {
				om.CoutTN("connect error. r = ", r, ". retry...");
			}
			else {
				om.CoutTN("connected.");
			}
		}

		// 连上之后，等 open 0 号服务 5 秒
		if (auto r = co_await p->WaitOpen(0, 5s)) {
			om.CoutTN("WaitOpen error. r = ", r, ". reconnect...");
			goto LabEnd;
		}
		else {
			om.CoutTN("WaitOpen success");
		}

		// 等到了 open，给 0 号服务 发 Ping，超时 15 秒
		om.CoutTN("SendRequest Ping");
		if (auto o = co_await p->SendRequest<Ping>(0, 500s, xx::NowSteadyEpoch10m()); !o) {
			om.CoutTN(p->Alive() ? "timeout!" : "stoped!");
			goto LabEnd;
		}
		else {
			switch (o.typeId()) {
			case xx::TypeId_v<Pong>: {
				auto ms = (xx::NowSteadyEpoch10m() - o.ReinterpretCast<Pong>()->ticks) / 1000.;
				om.CoutTN("receive Pong. delay = ", ms);
				break;
			}
			default:
				om.CoutTN("receive unhandled pkg = ", o);
				om.KillRecursive(o);
			}
		}

		// ...

	LabEnd:
		// 掐线 / clean up
		p->Stop();
		xx::CoutTN("stoped...");

		// 小睡一下，让别的协程处理下断线逻辑
		co_await xx::Timeout(1s);

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
