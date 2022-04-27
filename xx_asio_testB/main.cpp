// lobby + gateway + client -- gateway
#include "xx_asio_codes.h"

struct SPeer;
struct CPeer;
struct Server : xx::IOCCode<Server> {
	std::unordered_map<uint32_t, xx::Shared<SPeer>> serverPeers;
	std::unordered_map<uint32_t, xx::Shared<CPeer>> clientPeers;
	uint32_t clientAutoId = 0;
	int Run(asio::ip::address const& lobbyIP, uint16_t const& lobbyPort, uint16_t const& myPort);
};

template<typename...Args>
xx::Data MakeCommandData(Args const &... cmdAndArgs) {
	xx::Data d;
	d.Reserve(512);
	auto bak = d.WriteJump(sizeof(uint32_t));
	d.WriteFixed(0xFFFFFFFFu);
	d.Write(cmdAndArgs...);
	d.WriteFixedAt(bak, (uint32_t)(d.len - sizeof(uint32_t)));
	return d;
}

// 抄自 PeerRequestCode, 精简并去掉了注释. SPeer & CPeer 公用，使用 模板参数 来路由部分代码
// 包结构：4 字节长度 + 4 字节 target + data. target 如果为特殊值, 则路由到内部指令分支, 否则无脑转发
template<typename PeerDeriveType, bool isCPeer>
struct PeerHandleMessageCode {
	size_t HandleMessage(uint8_t* inBuf, size_t len) {
		if (PEERTHIS->stoping) return len;
		auto buf = (uint8_t*)inBuf;
		auto end = (uint8_t*)inBuf + len;
		uint32_t dataLen = 0;
		uint32_t target = 0;	// 重要的路由指示
		while (buf + sizeof(dataLen) <= end) {
			dataLen = *(uint32_t*)buf;
			if (dataLen > 1024 * 512) return 0;
			auto totalLen = sizeof(dataLen) + dataLen;
			if (buf + totalLen > end) break;
			do {
				auto dr = xx::Data_r(buf + sizeof(dataLen), dataLen);
				if (dr.ReadFixed(target)) return 0;
				//if (target == 0xFFFFFFFFu) {	// 内部指令
				//	if constexpr (isCPeer) {	// CPeer 逻辑
				//		PEERTHIS->ResetTimeout(20s);	// 重置超时时长( 续命 )
				//		PEERTHIS->Send({ buf, totalLen });	// echo back
				//		continue;
				//	}
				//	else {	// SPeer 逻辑
				//		if (PEERTHIS->HandleCommand(dr)) return 0;
				//	}
				//}
				//if (serverIds.find(target) == serverIds.end()) continue;	// 白名单? 找不到就忽略
				//auto&& sp = server.serverPeers[target];		// 指向目标服务 peer
				//if (!sp || !sp->Alive()) continue;	// 如果服务 peer 当前无效，则忽略
				//*(uint32_t*)(buf + 4) = clientId;	// 篡改当前包的 target 为 clientId
				//sp->Send({ buf, totalLen });	// 转发
				//PEERTHIS->ResetTimeout(20s);	// 重置超时时长( 续命 )
			} while (false);
			buf += totalLen;
		}
		return buf - inBuf;
	}
};

// 客户端连接上来时创建
struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, PeerHandleMessageCode<CPeer, false>, std::enable_shared_from_this<CPeer> {
	Server& server;
	CPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
		, clientId(++server_.clientAutoId)	// 填充 自增编号
	{
		ResetTimeout(20s);	// 设置超时时长
	}
	uint32_t clientId;		// 自增编号
	std::unordered_set<uint32_t> serverIds;	// 允许访问的 server peers 的 id 的白名单

	// 群发断开通知, 从容器移除
	void Kick();
	void Stop_();
};

// 用于连接到 lobby 服务并与之通信
struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, PeerHandleMessageCode<CPeer, true>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_)
		: PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{}
	
	uint32_t serverId = 0xFFFFFFFFu;	// 内部服务编号, 完成 Register 流程后填充( 同时也是有效性判断标志 )
	bool waitingPingBack = false;	// 等待对方回 ping 的标志位
	int64_t lastSendPingMS = 0;	// 最后一次发起 ping 的时间
	int64_t pingMS = 0;	// 收到回复后计算出来的 ping 值

	// 查找 & 返回 CPeer 的临时指针. 找不到 或者已 stoping stoped 返回 nullptr
	CPeer* TryGetCPeer(uint32_t const& clientId) {
		if (auto&& iter = server.clientPeers.find(clientId); iter == server.clientPeers.end()) return nullptr;
		else return iter->second->Alive() ? iter->second.pointer : nullptr;
	}

	int HandleCommand(xx::Data_r& dr) {
		std::string_view cmd;	// 试读取 cmd 字串. 失败直接断开
		if (int r = dr.Read(cmd)) return r;
		uint32_t clientId = 0;	// 公用 client id 容器
		if (cmd == "open"sv) {                        // 向客户端开放 serverId. 参数: clientId
			if (int r = dr.Read(clientId)) return r;	// 试读出 clientId. 失败直接断开
			if (auto&& cp = TryGetCPeer(clientId)) {	// 如果找到 client peer, 则转发
				cp->serverIds.emplace(serverId);	// 放入白名单
				cp->Send(MakeCommandData("open", serverId));	// 下发 open
			}
		}
		else if (cmd == "close"sv) {                // 关端口. 参数: clientId
			if (int r = dr.Read(clientId)) return r;	// 试读出 clientId. 失败直接断开
			if (auto&& cp = TryGetCPeer(clientId)) {	// 如果找到 client peer, 则 从白名单移除 下发 close
				cp->serverIds.erase(serverId);
				cp->Send(MakeCommandData("close", serverId));
			}
		}
		else if (cmd == "kick"sv) {                 // 踢玩家下线. 参数: clientId, delayMS
			int64_t delayMS = 0;
			if (int r = dr.Read(clientId, delayMS)) return r;
			auto&& cp = TryGetCPeer(clientId);	// 如果没找到 或已断开 则返回，忽略错误
			if (!cp) return 0;
			if (delayMS > 0) {	// 延迟踢下线
				cp->Send(MakeCommandData("close", (uint32_t)0));	// 下发一个 close 指令以便 client 收到后直接主动断开, 响应会比较快速
				cp->Kick();	// 移除
				cp->DelayStop(std::chrono::milliseconds(delayMS));	// 延迟掐
			}
			else {
				cp->Stop();	// 含 Kick
			}
		}
		else if (cmd == "ping"sv) {                 // ping 的回包. 不关心内容
			waitingPingBack = false;	// 清除 等回包 状态. 该状态在 pingtimer 中设置，并同时发送 ping 指令
			pingMS = xx::NowSteadyEpochMilliseconds() - lastSendPingMS;	// 计算延迟, 便于统计
		}
		else {
			return __LINE__;
		}
		return 0;
	}

	// 从所有 client peers 里的白名单中移除, 下发 close, 从 serverPeers 移除自生
	void Stop_() {
		auto d = MakeCommandData("close", serverId);
		for (auto&& kv : server.clientPeers) {
			kv.second->serverIds.erase(serverId);
			kv.second->Send(xx::Data(d));
		}
		server.serverPeers[serverId].Reset();
	}

	// 起协程, 开始 Register 流程, 完成时填充 serverId
	void Start_() {
		// todo
	}
};

void CPeer::Kick() {
	if (auto iter = server.clientPeers.find(clientId); iter != server.clientPeers.end()) {
		auto d = MakeCommandData("close", clientId);
		for (auto&& sid : serverIds) {
			if (auto&& sp = server.serverPeers[sid]; sp && sp->Alive()) {
				sp->Send(xx::Data(d));
			}
		}
		server.clientPeers.erase(iter);
	}
}
void CPeer::Stop_() {
	Kick();
}

int Server::Run(asio::ip::address const& lobbyIP, uint16_t const& lobbyPort, uint16_t const& myPort) {
	std::cout << "lobby + gateway + client -- gateway running... port = 54322" << std::endl;
	Listen<CPeer>(myPort);

	co_spawn(ioc, [this, lobbyIP, lobbyPort]()->awaitable<void> {

		// 根据配置，在 serverPeers 中 创建 peer 并不断 Connect 直到连上

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
