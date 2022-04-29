// lobby + gateway + client -- gateway
#include "xx_asio_codes.h"

struct SPeer;
struct CPeer;
struct Server : xx::IOCCode<Server> {
	std::unordered_map<uint32_t, std::shared_ptr<SPeer>> serverPeers;
	std::unordered_map<uint32_t, std::shared_ptr<CPeer>> clientPeers;
	uint32_t clientAutoId = 0;	// 每次客户端 peer 创建时自增填充到其 clientId
	uint32_t gatewayId = 1;	// 当前服务id. 应该根据配置来填充
	int Run(asio::ip::address lobbyIP, uint16_t lobbyPort, uint16_t myPort);
	CPeer* TryGetCPeer(uint32_t clientId);	// 查找 & 返回 CPeer 的临时指针. 找不到 或者已 stoping stoped 返回 nullptr
	SPeer* TryGetSPeer(uint32_t serverId);	// 查找 & 返回 SPeer 的临时指针. 找不到 或者已 stoping stoped 返回 nullptr
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

	void Start_();	// 放入容器, 给 0 号服务器发 "accept" + clientId + "ip:port"
	int HandleData(xx::Data_r&& dr);	// 根据 target 找到 SPeer 转发
	void TryErase();	// 群发断开通知, 从容器移除
	void Stop_();	// TryErase()
};

// 用于连接到 lobby 服务并与之通信
struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, xx::PeerHandleMessageCode<CPeer>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_)
		: PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{
	}
	uint32_t serverId = 0xFFFFFFFFu;	// 服务编号, 创建时根据配置填充
	bool waitingPingBack = false;	// 等待对方回 ping 的标志位
	int64_t lastSendPingMS = 0;	// 最后一次发起 ping 的时间
	int64_t pingMS = 0;	// 收到回复后计算出来的 ping 值( 用于统计? )

	// 发送 register serverId
	void Start_() {
		assert(serverId != 0xFFFFFFFFu);
		Send(xx::MakeCommandData("gatewayId"sv, server.gatewayId));
		co_spawn(ioc, [this, self = shared_from_this()]()->awaitable<void> {
			while (Alive()) {
				co_await xx::Timeout(1000ms);	// 避免无脑空转，省点 cpu
				auto&& now = xx::NowSteadyEpochMilliseconds();	// 取当前 时间点ms
				if (!waitingPingBack) {	// 如果需要发 ping, 就发
					waitingPingBack = true;	// 记录状态
					lastSendPingMS = now;	// 记录发ping时的时间点
					Send(xx::MakeCommandData("ping"sv, now));
				}
				else {	// 如果正在等 ping 回应
					if (now - lastSendPingMS > 5000) Stop();	// 超时 5 秒 就掐线
				}
			}
		}, detached);
	}
	// PeerHandleMessageCode 会在包切片后调用
	// 根据 target 找到 CPeer 转发 或执行 内部指令, 非 0xFFFFFFFFu 则为正常数据，走转发流，否则走内部指令
	int HandleData(xx::Data_r&& dr) {
		uint32_t target;
		if (int r = dr.ReadFixed(target)) return __LINE__;		// 试读取 target. 失败直接断开
		if (target != 0xFFFFFFFFu) {
			if (auto&& cp = server.TryGetCPeer(target)) {	//  试找到 client peer 转发
				*(uint32_t*)(dr.buf + 4) = serverId;	// 篡改 clientId 为 serverId
				cp->Send(dr);
			}
		}
		else {	// 内部指令
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
					cp->serverIds.erase(serverId);	// 从白名单移除
					cp->Send(xx::MakeCommandData("close", serverId));	// 下发 close
				}
			}
			else if (cmd == "kick"sv) {	// 踢玩家下线. 参数: clientId, delayMS
				int64_t delayMS = 0;
				if (int r = dr.Read(clientId, delayMS)) return __LINE__;
				if (auto&& cp = server.TryGetCPeer(clientId)) {	// 如果没找到 或已断开 则返回，忽略错误
					if (delayMS > 0) {	// 延迟踢下线
						cp->Send(xx::MakeCommandData("close", (uint32_t)0));	// 下发一个 close 指令以便 client 收到后直接主动断开, 响应会比较快速
						cp->TryErase();	// 移除
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
		return 0;
	}
	// 从所有 client peers 里的白名单中移除, 下发 close, 从 serverPeers 移除自生
	void Stop_() {
		auto d = xx::MakeCommandData("close"sv, serverId);
		for (auto&& kv : server.clientPeers) {
			if (auto iter = kv.second->serverIds.find(serverId); iter != kv.second->serverIds.end()) {
				kv.second->serverIds.erase(iter);
				kv.second->Send(xx::Data(d));
			}
		}
		server.serverPeers.erase(serverId);
	}
};

CPeer* Server::TryGetCPeer(uint32_t clientId) {
	if (auto&& iter = clientPeers.find(clientId); iter == clientPeers.end()) return nullptr;
	else return iter->second->Alive() ? iter->second.get() : nullptr;
}

SPeer* Server::TryGetSPeer(uint32_t serverId) {
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
	if (auto sp = server.TryGetSPeer(0)) {	// 如果定位到 0 号服务器, 就发 accept 内部指令
		auto ep = socket.remote_endpoint();
		sp->Send(xx::MakeCommandData("accept"sv, clientId, xx::RemoteEndPointToString(socket)));
		server.clientPeers.emplace(clientId, shared_from_this());
	}
	else {
		Stop();
	}
}

void CPeer::TryErase() {
	if (auto iter = server.clientPeers.find(clientId); iter != server.clientPeers.end()) {	// 防重复执行
		auto d = xx::MakeCommandData("close"sv, clientId);
		for (auto&& sid : serverIds) {
			if (auto&& sp = server.TryGetSPeer(sid)) {
				sp->Send(xx::Data(d));
			}
		}
		server.clientPeers.erase(iter);
	}
}

void CPeer::Stop_() {
	TryErase();
}

int Server::Run(asio::ip::address lobbyIP, uint16_t lobbyPort, uint16_t myPort) {
	Listen<CPeer>(myPort);
	co_spawn(ioc, [this, lobbyIP, lobbyPort]()->awaitable<void> {
		while (true) {	// 守护协程: 根据配置，在 serverPeers 中 创建 speer 并不断 Connect 直到连上( 已断开的 speer 在 stop 时会将自己从 serverPeers 中移除 )
			uint32_t serverId = 0;	// 模拟从配置读出了一个网关需要连的 serverid
			uint32_t gatewayId = 1;	// 模拟从配置读出了自己的 gatewayid
			auto&& p = serverPeers[serverId];	// 创建 或/和 引用到 SPeer 的存放点
			if (!p) {	// 如果存放点为空
				p = std::make_shared<SPeer>(*this);	// 创建( 初始状态为 stoped, 不会执行 Start )
				p->serverId = serverId;	// 填充 服务 id
			}
			if (p->stoped) {	// 如果没连上，就开始连. 
				if (int r = co_await p->Connect(lobbyIP, lobbyPort, 2s)) {	// 开始连接. 超时 2 秒
					std::cout << "!"; std::cout.flush();	// 随便输出点啥，体现异常
				}
			}	// 连上后将自动执行 SPeer 的 Start, 启动协程, 发出 gatewayId, 并不断的 ping
			std::cout << "."; std::cout.flush();	// 随便输出点啥，显得没死机
			co_await xx::Timeout(1000ms);	// 避免无脑空转，省点 cpu
		}
	}, detached);
	ioc.run();
	return 0;
}

int main() {
	std::cout << "lobby + gateway + client -- gateway running... this port = 54322, connect to lobby port = 54321"sv << std::endl;
	return Server().Run(asio::ip::address::from_string("127.0.0.1"), 54321, 54322);
}
