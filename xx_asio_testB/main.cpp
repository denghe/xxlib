// lobby + gateway + client -- gateway
#include "xx_asio_codes.h"

struct SPeer;
struct CPeer;
struct Server : xx::IOCCode<Server> {
	std::unordered_map<uint32_t, std::shared_ptr<SPeer>> serverPeers;
	std::unordered_map<uint32_t, std::shared_ptr<CPeer>> clientPeers;
	uint32_t clientAutoId = 0;
	int Run(asio::ip::address const& lobbyIP, uint16_t const& lobbyPort, uint16_t const& myPort);
	CPeer* TryGetCPeer(uint32_t const& clientId);	// 查找 & 返回 CPeer 的临时指针. 找不到 或者已 stoping stoped 返回 nullptr
	SPeer* TryGetSPeer(uint32_t const& serverId);	// 查找 & 返回 SPeer 的临时指针. 找不到 或者已 stoping stoped 返回 nullptr
};

// 客户端连接上来时创建
struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerHandleMessageCode<CPeer>, std::enable_shared_from_this<CPeer> {
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

	int HandleData(xx::Data_r&& dr);

	void Start_();	// 放入容器
	void Kick();	// 群发断开通知, 从容器移除
	void Stop_();
};

// 用于连接到 lobby 服务并与之通信
struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, xx::PeerHandleMessageCode<CPeer>, std::enable_shared_from_this<SPeer> {
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

	int HandleData(xx::Data_r&& dr) {
		uint32_t target;	// 试读取 cmd 字串. 失败直接断开
		if (int r = dr.ReadFixed(target)) return __LINE__;
		if (target == 0xFFFFFFFFu) {	// 内部指令
			std::string_view cmd;	// 试读取 cmd 字串. 失败直接断开
			if (int r = dr.Read(cmd)) return __LINE__;
			uint32_t clientId = 0;	// 公用 client id 容器
			if (cmd == "open"sv) {	// 向客户端开放 serverId. 参数: clientId
				if (int r = dr.Read(clientId)) return __LINE__;	// 试读出 clientId. 失败直接断开
				if (auto&& cp = server.TryGetCPeer(clientId)) {	// 如果找到 client peer, 则转发
					cp->serverIds.emplace(serverId);	// 放入白名单
					cp->Send(xx::MakeCommandData("open", serverId));	// 下发 open
				}
			}
			else if (cmd == "close"sv) {	// 关端口. 参数: clientId
				if (int r = dr.Read(clientId)) return __LINE__;	// 试读出 clientId. 失败直接断开
				if (auto&& cp = server.TryGetCPeer(clientId)) {	// 如果找到 client peer, 则 从白名单移除 下发 close
					cp->serverIds.erase(serverId);
					cp->Send(xx::MakeCommandData("close", serverId));
				}
			}
			else if (cmd == "kick"sv) {	// 踢玩家下线. 参数: clientId, delayMS
				int64_t delayMS = 0;
				if (int r = dr.Read(clientId, delayMS)) return __LINE__;
				if (auto&& cp = server.TryGetCPeer(clientId)) {	// 如果没找到 或已断开 则返回，忽略错误
					if (delayMS > 0) {	// 延迟踢下线
						cp->Send(xx::MakeCommandData("close", (uint32_t)0));	// 下发一个 close 指令以便 client 收到后直接主动断开, 响应会比较快速
						cp->Kick();	// 移除
						cp->DelayStop(std::chrono::milliseconds(std::max((uint64_t)delayMS, 10000ull)));	// 延迟掐( 转无符号 限制最大值 防止调用者傻屄 )
					}
					else {
						cp->Stop();	// 立即掐, 含 Kick
					}
				}
			}
			else if (cmd == "ping"sv) {	// ping 的回包. 不关心内容
				waitingPingBack = false;	// 清除 等回包 状态. 该状态在 pingtimer 中设置，并同时发送 ping 指令
				pingMS = xx::NowSteadyEpochMilliseconds() - lastSendPingMS;	// 计算延迟, 便于统计
			}
			else {
				return __LINE__;
			}
		}
		else {
			if (auto&& cp = server.TryGetCPeer(target)) {	// 如果找到 client peer, 则转发
				*(uint32_t*)(dr.buf + 4) = serverId;	// 篡改 clientId 为 serverId
				cp->Send(dr);	// 转发
			}
		}
		return 0;
	}

	// 从所有 client peers 里的白名单中移除, 下发 close, 从 serverPeers 移除自生
	void Stop_() {
		auto d = xx::MakeCommandData("close", serverId);
		for (auto&& kv : server.clientPeers) {
			kv.second->serverIds.erase(serverId);
			kv.second->Send(xx::Data(d));
		}
		server.serverPeers.erase(serverId);
	}

	// 起协程, 开始 Register 流程, 完成时填充 serverId
	void Start_() {
		// todo
	}
};

CPeer* Server::TryGetCPeer(uint32_t const& clientId) {
	if (auto&& iter = clientPeers.find(clientId); iter == clientPeers.end()) return nullptr;
	else return iter->second->Alive() ? iter->second.get() : nullptr;
}

SPeer* Server::TryGetSPeer(uint32_t const& serverId) {
	if (auto&& iter = serverPeers.find(serverId); iter == serverPeers.end()) return nullptr;
	else return iter->second->Alive() ? iter->second.get() : nullptr;
}

int CPeer::HandleData(xx::Data_r&& dr) {
	uint32_t target;	// 试读取 cmd 字串. 失败直接断开
	if (int r = dr.ReadFixed(target)) return __LINE__;
	if (target == 0xFFFFFFFFu) {	// 内部指令
		ResetTimeout(20s);	// 重置超时时长( 续命 )
		Send(dr);	// echo back
	}
	else {
		if (serverIds.find(target) == serverIds.end()) return 0;	// 白名单? 找不到就忽略
		if (auto&& sp = server.TryGetSPeer(target)) {	// 拿有效服务 peer
			ResetTimeout(20s);	// 重置超时时长( 续命 )
			*(uint32_t*)(dr.buf + 4) = clientId;	// 篡改当前包的 target 为 clientId
			sp->Send(dr);	// 转发
		}
	}
	return 0;
}

void CPeer::Start_() {
	// todo: 给 0 号服务器发通知 ?
	if (auto sp = server.TryGetSPeer(0)) {
		sp->Send(xx::MakeCommandData("", clientId));	// todo
		server.clientPeers.emplace(clientId, shared_from_this());
	}
	else {
		Stop();
	}
}

void CPeer::Kick() {
	if (auto iter = server.clientPeers.find(clientId); iter != server.clientPeers.end()) {
		auto d = xx::MakeCommandData("close", clientId);
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
