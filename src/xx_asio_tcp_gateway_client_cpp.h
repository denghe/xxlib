#pragma once
#include <xx_asio_codes.h>

// 为 cpp 实现一个带 拨号 与 通信 的 tcp 网关 协程 client, 基于帧逻辑, 无需继承, 开箱即用

namespace xx::Asio::Tcp::Gateway::Cpp {

	// 收到的数据容器( 含 lua 所有, cpp push + request 类 )
	using ReceivedData = std::tuple<uint32_t, int32_t, xx::Data>;
	using ReceivedObject = std::tuple<uint32_t, int32_t, xx::ObjBase_s>;

	struct CPeer;
	struct Client : xx::IOCCode<Client> {
		xx::ObjManager om;
		std::shared_ptr<CPeer> peer;																// 当前 peer
		std::unordered_set<uint32_t> cppServerIds;													// 属于 cpp 处理的 serverId 存放于此

		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充

		void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口

		template<typename...Args>
		void AddCppServerIds(Args...ids) {															// 添加一到多个 cpp 服务 id
			(cppServerIds.insert(ids), ...);
		}

		void Update();																				// 每帧开始和逻辑结束时 call 一次

		void Reset();																				// 老 peer Stop + 新建 peer

		operator bool() const;																		// 检测连接有效性. peer && peer->Alive()

		awaitable<int> Dial();																		// 拨号. 成功连上返回 0


		template<typename...Args>
		awaitable<bool> WaitOpens(std::chrono::steady_clock::duration d, Args...ids);				// 带超时同时等一批 server id open. 超时返回 false

		bool TryPop(ReceivedObject& ro);
		bool TryPop(ReceivedData& rd);

		//awaitable<bool> Pop( handler )
		// 
		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendResponse(Args const& ... args);													//

		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendPush(Args const& ... args);														//

		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendRequest(Args const& ... args);													//
	};

	struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestTargetCode<CPeer>, std::enable_shared_from_this<CPeer> {
		Client& client;
		CPeer(Client& client_)
			: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
			, PeerTimeoutCode(client_.ioc)
			, PeerRequestTargetCode(client_.om)
			, client(client_)
			, openWaiter(client_.ioc)
		{
			ResetTimeout(15s);
		}

		std::deque<ReceivedData> recvDatas;															// 已收到的 lua( all ) 数据包集合
		std::deque<ReceivedObject> recvObjs;														// 已收到的 cpp(request, push) 数据包集合

		std::unordered_set<uint32_t> openServerIds;													// 已 open 的 serverId 存放于此

		asio::steady_timer openWaiter;																// open 等待器
		std::vector<uint32_t> openWaitServerIds;													// 参与等待的 id 列表

		void Stop_() {
			recvDatas.clear();
			recvObjs.clear();
			openWaiter.cancel();																	// 取消某些等待器
		}

		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
			ResetTimeout(15s);
			if (target == 0xFFFFFFFF) return ReceveCommand(dr);										// 收到 command. 转过去处理
			int32_t serial;																			// 读出序号. 出错 返回负数行号( 掐线 )
			if (dr.Read(serial)) return -__LINE__;
			if (client.cppServerIds.contains(target)) return 1; 									// 属于 cpp 的: passthrough
			recvDatas.emplace_back(target, serial, xx::Data(dr.buf + dr.offset, dr.len - dr.offset));	// 属于 lua: 放入 data 容器
			return 0;																				// 返回 已处理
		}
		int ReceveCommand(xx::Data_r& dr) {
			std::string_view cmd;
			if (dr.Read(cmd)) return -__LINE__;
			if (cmd == "open"sv) {																	// 收到 打开服务 指令
				uint32_t serviceId = -1;
				if (dr.Read(serviceId)) return -__LINE__;
				openServerIds.insert(serviceId);													// 添加到 open列表
				if (!openWaitServerIds.empty()) {													// 判断是否正在 等open
					if (CheckOpens()) {																// 全 open 了: 清除参数 & cancel 等待器
						openWaiter.cancel();
					}
				}
			}
			else if (cmd == "close"sv) {															// 收到 关闭服务 指令
				uint32_t serviceId = -1;
				if (dr.Read(serviceId)) return -__LINE__;
				//if (serviceId == 0) return -__LINE__;												// 0 号服务关闭：返回负数( 掐线 )
				openServerIds.erase(serviceId);														// 从 open列表 移除
				if (openServerIds.empty()) return -__LINE__;										// 所有服务都关闭了：返回负数( 掐线 )
			}
			else return 0;																			// 网关 echo 回来的东西 todo：计算到网关的 ping 值?
			return 1;																				// 返回 已处理
		}
		int ReceiveTargetRequest(uint32_t target, int32_t serial, xx::ObjBase_s&& o_) {
			recvObjs.emplace_back(target, serial, std::move(o_));									// 放入 recvObjs
			return 0;
		}
		int ReceiveTargetPush(uint32_t target, xx::ObjBase_s && o_) {
			recvObjs.emplace_back(target, 0, std::move(o_));										// 放入 recvObjs
			return 0;
		}

		bool CheckOpens() {																			// openWaitServerIds 所有 id 都 open 则返回 true
			for (auto& id : openWaitServerIds) {
				if (!openServerIds.contains(id)) return false;
			}
			openWaitServerIds.clear();																// 成功后清空参数
			return true;
		}
	};

	inline void Client::SetDomainPort(std::string_view domain_, uint16_t port_) {
		domain = domain_;
		port = port_;
	}

	inline void Client::Update() {
		ioc.poll_one();
	}

	inline Client::operator bool() const {
		return peer && peer->Alive();
	}

	inline void Client::Reset() {
		if (peer) {
			peer->Stop();
		}
		peer = std::make_shared<CPeer>(*this);														// 每次新建 容易控制 协程生命周期, 不容易出问题
	}

	inline awaitable<int> Client::Dial() {
		if (domain.empty()) co_return __LINE__;
		if (!port) co_return __LINE__;
		Reset();
		asio::ip::address addr;
		do {
			asio::ip::tcp::resolver resolver(ioc);													// 创建一个域名解析器
			auto rr = co_await(resolver.async_resolve(domain, ""sv, use_nothrow_awaitable) || xx::Timeout(5s));	// 开始解析, 得到一个 variant 包含两个协程返回值
			if (rr.index() == 1) co_return __LINE__;												// 如果 variant 存放的是第 2 个结果, 那就是 超时
			auto& [e, rs] = std::get<0>(rr);														// 展开第一个结果为 err + results
			if (e) co_return __LINE__;																// 出错: 解析失败
			auto iter = rs.cbegin();																// 拿到迭代器，指向首个地址
			if (auto idx = (int)((size_t)xx::NowEpochMilliseconds() % rs.size())) {					// 根据当前 ms 时间点 随机选一个下标
				std::advance(iter, idx);															// 快进迭代器
			}
			addr = iter->endpoint().address();														// 从迭代器获取地址
		} while (false);
		if (auto r = co_await peer->Connect(addr, port, 5s)) co_return __LINE__;					// 开始连接. 超时 5 秒
		co_return 0;
	}

	inline bool Client::TryPop(ReceivedObject& ro) {
		if (!*this || peer->recvObjs.empty()) return false;
		ro = std::move(peer->recvObjs.front());
		peer->recvObjs.pop_front();
		return true;
	}
	inline bool Client::TryPop(ReceivedData& rd) {
		if (!*this || peer->recvDatas.empty()) return false;
		rd = std::move(peer->recvDatas.front());
		peer->recvDatas.pop_front();
		return true;
	}

	template<typename...Args>
	awaitable<bool> Client::WaitOpens(std::chrono::steady_clock::duration d, Args...ids) {
		if (!*this) co_return false;															// 异常：已断开
		if (!peer->openWaitServerIds.empty()) co_return false;									// 异常：正在等??
		(peer->openWaitServerIds.push_back(ids), ...);											// 存储参数
		if (peer->CheckOpens()) co_return true;													// 已 open: 返回 true
		peer->openWaiter.expires_after(d);														// 初始化 timer 超时时长
		co_await peer->openWaiter.async_wait(use_nothrow_awaitable);							// 开始等待
		if (!*this) co_return false;															// 异常：已断开
		if (peer->openWaitServerIds.empty()) co_return true;									// 如果 cmd open 那边 CheckOpens true 会清空参数, 返回 true
		peer->openWaitServerIds.clear();														// 超时, 清空参数, 不再等待. 返回 false
		return false;
	}


	//template<typename PKG, typename ... Args>
	//inline void Client::Send(Args const& ... args) {
	//	if (!peer || !peer->Alive()) return;
	//	peer->SendPush<PKG>(args...);																// 转发到 peer 的 SendPush 函数
	//}

}


// for lua

// AddCppServiceId
// int SendEcho(xx::Data data);
// int SendTo(int serviceId, int serial, xx::Data data) 
