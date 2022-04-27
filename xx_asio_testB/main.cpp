// lobby + gateway + client -- gateway
#include "xx_asio_codes.h"

struct SPeer;
struct CPeer;
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;
	std::unordered_map<uint32_t, xx::Shared<SPeer>> serverPeers;
	std::unordered_map<uint32_t, xx::Shared<CPeer>> clientPeers;
	uint32_t clientAutoId = 0;
	int Run(asio::ip::address const& lobbyIP, uint16_t const& lobbyPort, uint16_t const& myPort);
};

// 客户端连接上来时创建
struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, std::enable_shared_from_this<CPeer> {
	Server& server;
	CPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{}

	// 抄自 PeerRequestCode, 精简并去掉了注释
	// 包结构：4 字节长度 + 4 字节 target + data. target 如果为特殊值, 则路由到内部指令分支, 否则无脑转发
	size_t HandleMessage(uint8_t* inBuf, size_t len) {
		if (stoping) return len;

		auto buf = (uint8_t*)inBuf;
		auto end = (uint8_t*)inBuf + len;
		uint32_t dataLen = 0, target = 0;

		while (buf + sizeof(dataLen) <= end) {
			dataLen = *(uint32_t*)buf;
			if (dataLen > 1024 * 512) return 0;
			auto totalLen = sizeof(dataLen) + dataLen;
			if (buf + totalLen > end) break;
			do {
				auto dr = xx::Data_r(buf + sizeof(dataLen), dataLen);
				if (dr.ReadFixed(target)) return 0;
				// todo
			} while (false);
			buf += totalLen;
		}
		return buf - inBuf;
	}
};

// 用于连接到 lobby 服务并与之通信
struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, xx::PeerRequestCode<SPeer, true>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_)
		: PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{}

	int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
		if (target == 0xFFFFFFFF) {
			std::string_view cmd;
			if (dr.Read(cmd)) return -__LINE__;	// error
			uint32_t serviceId;
			if (cmd == "open"sv) {
				if (dr.Read(serviceId)) return -__LINE__;	// read error
				//openServerIds.insert(serviceId);
			}
			else if (cmd == "close"sv) {
				if (dr.Read(serviceId)) return __LINE__;	// read error
				//openServerIds.erase(serviceId);
			}
			else return -__LINE__;	// cmd error: unknown cmd received
			return 0;	// handled: success
		}
		return 1;	// passthrough
	}
};

int Server::Run(asio::ip::address const& lobbyIP, uint16_t const& lobbyPort, uint16_t const& myPort) {
	std::cout << "lobby + gateway + client -- gateway running... port = 54322" << std::endl;
	Listen<CPeer>(myPort);

	co_spawn(ioc, [this, lobbyIP, lobbyPort]()->awaitable<void> {

		//	// 创建个 peer 连接到 lobby
		//	auto p = std::make_shared<SPeer>(*this);

		//LabBegin:
		//	// 如果没连上，就反复的连
		//	while (p->stoped) {
		//		// 开始连接. 超时 1 秒
		//		if (auto r = co_await p->Connect(lobbyIP, lobbyPort, 1s)) {
		//			om.CoutN("Connect error. r = ", r, ". retry...");
		//		}
		//	}

		//	// 连上之后，发注册信息
		//	// todo

		//	// 断线检测
		//	while (p->Alive()) {
		//		co_await xx::Timeout(1s);
		//		std::cout << ".";
		//		std::cout.flush();
		//	}

		//LabEnd:
		//	// 掐线
		//	p->Stop();

			// 小睡
			co_await xx::Timeout(2s);

		//	// 重连
		//	goto LabBegin;

		}, detached);

	ioc.run();
	return 0;
}

int main() {
	return Server().Run(asio::ip::address::from_string("127.0.0.1"), 54321, 54322);
}
