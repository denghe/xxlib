#pragma once
#include <xx_asio_codes.h>

// 为 cpp 实现一个带 拨号 与 通信 的 tcp 网关 协程 client, 基于帧逻辑, 无需继承, 开箱即用

namespace xx::Asio::Tcp::Gateway {
	
	// 收到的数据容器 T == Data: for script     T == xx::ObjBase_s for c++
	template<typename T>
	struct Package {
		XX_OBJ_STRUCT_H(Package);
		uint32_t serverId = 0;
		int32_t serial = 0;
		T data;
		Package(uint32_t const& serverId, int32_t const& serial, T&& data) : serverId(serverId), serial(serial), data(std::move(data)) {}
	};

	struct CPeer;
	struct Client : IOCCode<Client> {
        typedef asio::io_context::executor_type ExecutorType;
        asio::executor_work_guard<ExecutorType> work_guard_;
        Client() : work_guard_(asio::make_work_guard(ioc)) {}
		ObjManager om;
		std::shared_ptr<CPeer> peer;																// 当前 peer
		uint32_t cppServerId = 0xFFFFFFFF;															// 属于 cpp 处理的 serverId 存放于此

		std::string secret;																			// 用于数据滚动异或
		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充
		bool busy = false;																			// 繁忙标志 for lua ( lua bind 函数在 co_spawn 之前 check & set, 之后 clear )

		void SetSecret(std::string_view secret);													// 设置数据滚动异或串

		void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口

		void SetCppServerId(uint32_t id);															// 设置 cpp 服务 id

		void Update();																				// 每帧开始和逻辑结束时 call 一次

		void Reset();																				// 老 peer Stop + 新建 peer

		operator bool() const;																		// 检测连接有效性. peer && peer->Alive()

		awaitable<int> Dial();																		// 拨号. 成功连上返回 0

		template<typename...Args>
		awaitable<bool> WaitOpens(std::chrono::steady_clock::duration d, Args...ids);				// 带超时同时等一批 server id open. 超时返回 false

		bool IsOpened(uint32_t serverId);															// 检查当前某 serverId 是否已 open

		template<typename T = ObjBase>
		bool TryPop(Package<Shared<T>>& ro, std::optional<uint32_t> serverId = std::nullopt);		// 试获取一条消息内容. 没有则返回 false. 类型不符 填 null. ObjBase 适配所有类型. 如果有传 serverId 就限定获取

		bool TryPop(Package<Data>& rd);																// 试获取一条消息数据. 没有则返回 false

		template<typename T = ObjBase>
		awaitable<bool> Pop(std::chrono::steady_clock::duration d, Package<Shared<T>>& ro, std::optional<uint32_t> serverId = std::nullopt);	// 带超时 等待 接收一条消息. 超时返回 false. 类型不符也返回 false. 如果有传 serverId 就限定获取

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponseTo(uint32_t const& target, int32_t const& serial, Args const& ... args);	// 转发到 peer-> 同名函数

		template<typename PKG = ObjBase, typename ... Args>
		void SendPushTo(uint32_t const& target, Args const& ... args);								// 转发到 peer-> 同名函数

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequestTo(uint32_t const& target, std::chrono::steady_clock::duration d, Args const& ... args);	// 转发到 peer-> 同名函数

		void SendTo(uint32_t const& target, int32_t const& serial, Span const& d);					// 转发到 peer-> Send( MakeData ... ) ( for lua )
		void Send(Data && d);																		// 转发到 peer-> 同名函数 ( for lua )

		// for cpp easy use

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponse(int32_t const& serial, Args const& ... args);								// SendResponseTo( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		void SendPush(Args const& ... args);														// SendPush( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequest(Args const& ... args);										// SendRequest( cppServerId, 15s, ...
	};

	struct CPeer : PeerCode<CPeer>, PeerTimeoutCode<CPeer>, PeerRequestTargetCode<CPeer>, std::enable_shared_from_this<CPeer> {
		Client& client;
		CPeer(Client& client_)
			: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
			, PeerTimeoutCode(client_.ioc)
			, PeerRequestTargetCode(client_.om)
			, client(client_)
			, openWaiter(client_.ioc)
			, objectWaiter(client_.ioc)
		{
			ResetTimeout(15s);
		}

		std::deque<Package<Data>> recvDatas;														// 已收到的 lua( all ) 数据包集合
		std::unordered_map<uint32_t, std::deque<Package<ObjBase_s>>> recvObjs;						// 已收到的 cpp(request, push) 数据包集合. 按照 serverId 分组存储

		std::unordered_set<uint32_t> openServerIds;													// 已 open 的 serverId 存放于此

		asio::steady_timer openWaiter;																// open 等待器
		std::vector<uint32_t> openWaitServerIds;													// 参与等待的 id 列表

		asio::steady_timer objectWaiter;															// 数据包等待器
		bool waitingObject = false;																	// 数据包等待状态

		void Stop_() {																				// 所有环境变量恢复初始状态, 取消所有等待
			recvDatas.clear();
			recvObjs.clear();
			openServerIds.clear();
			openWaitServerIds.clear();
			waitingObject = false;
			openWaiter.cancel();
			objectWaiter.cancel();
		}

		// 消息滚动异或解密
		void PrehandleData(uint8_t* b, size_t len) {
			if (auto& s = client.secret; s.size()) {
				xx::XorContent(s.data(), s.size(), (char*)b + 4, len - 4);
			}
		}
		// 消息滚动异或加密
		void Presend(xx::Data& d) {
			PrehandleData(d.buf, d.len);															// 因为是异或逻辑，加密直接 call 上面的函数即可
		}

		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, Data_r& dr) {
			ResetTimeout(15s);
			if (target == 0xFFFFFFFF) return ReceveCommand(dr);										// 收到 command. 转过去处理
			if (client.cppServerId == target) return 1; 											// 属于 cpp 的: passthrough
			int32_t serial;																			// 读出序号. 出错 返回负数行号( 掐线 )
			if (dr.Read(serial)) return -__LINE__;
			recvDatas.emplace_back(target, serial, Data(dr.buf + dr.offset, dr.len - dr.offset));	// 属于 lua: 放入 data 容器
			return 0;																				// 返回 已处理
		}
		int ReceveCommand(Data_r& dr) {
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
			//else																					// 网关 echo 回来的东西 todo：计算到网关的 ping 值?
			return 0;																				// 返回 已处理
		}
		int ReceiveTargetRequest(uint32_t target, int32_t serial, ObjBase_s&& o_) {
			recvObjs[target].emplace_back(target, serial, std::move(o_));							// 放入 recvObjs
			if (waitingObject) {																	// 如果发现正在等包
				waitingObject = false;																// 清理标志位
				objectWaiter.cancel();																// 放行
			}
			return 0;
		}
		int ReceiveTargetPush(uint32_t target, ObjBase_s && o_) {
			return ReceiveTargetRequest(target, 0, std::move(o_));
		}

		bool CheckOpens() {																			// openWaitServerIds 所有 id 都 open 则返回 true
			for (auto& id : openWaitServerIds) {
				if (!openServerIds.contains(id)) return false;
			}
			openWaitServerIds.clear();																// 成功后清空参数
			return true;
		}
	};

	inline void Client::SetCppServerId(uint32_t id) {
		cppServerId = id;
	}

	inline void Client::SetSecret(std::string_view secret_) {
		secret = secret_;
	}

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
			auto rr = co_await(resolver.async_resolve(domain, ""sv, use_nothrow_awaitable) || Timeout(5s));	// 开始解析, 得到一个 variant 包含两个协程返回值
			if (rr.index() == 1) co_return __LINE__;												// 如果 variant 存放的是第 2 个结果, 那就是 超时
			auto& [e, rs] = std::get<0>(rr);														// 展开第一个结果为 err + results
			if (e) co_return __LINE__;																// 出错: 解析失败
			auto iter = rs.cbegin();																// 拿到迭代器，指向首个地址
			if (auto idx = (int)((size_t)NowEpochMilliseconds() % rs.size())) {						// 根据当前 ms 时间点 随机选一个下标
				std::advance(iter, idx);															// 快进迭代器
			}
			addr = iter->endpoint().address();														// 从迭代器获取地址
		} while (false);
		if (auto r = co_await peer->Connect(addr, port, 5s)) co_return __LINE__;					// 开始连接. 超时 5 秒
		co_return 0;
	}

	inline bool Client::IsOpened(uint32_t serverId) {
		if (!*this || peer->openServerIds.empty()) return false;
		return peer->openServerIds.contains(serverId);
	}

	inline bool Client::TryPop(Package<Data>& rd) {
		if (!*this || peer->recvDatas.empty()) return false;
		rd = std::move(peer->recvDatas.front());
		peer->recvDatas.pop_front();
		return true;
	}

	template<typename T>
	inline bool Client::TryPop(Package<Shared<T>>& ro, std::optional<uint32_t> serverId) {
		if (!*this) return false;																	 // 异常：已断开
		std::deque<Package<ObjBase_s>>* os = nullptr;
		if (serverId.has_value()) {																	 // 如果有指定 serverId
			os = &peer->recvObjs[serverId.value()];													 // 定位到队列
			if (os->empty()) return false;															 // 空: 短路出去
		}
		else {
			for (auto& kv : peer->recvObjs) {														// 随便找出一个有数据的队列
				if (!kv.second.empty()) {
					os = &kv.second;
					break;
				}
			}
			if (!os) return false;																	// 都没有数据
		}
		auto& o = os->front();
		if constexpr (std::is_same_v<T, ObjBase>) {
			ro = std::move(o);
		} else {
			static_assert(std::is_base_of_v<ObjBase, T>);
			if (o.data.GetTypeId() == xx::TypeId_v<T>) {
				ro = std::move(reinterpret_cast<Package<Shared<T>>&>(o));
			} else {
				ro.serverId = 0;
				ro.serial = 0;
				ro.data.Reset();
			}
		}
		os->pop_front();
		return true;
	}

	template<typename T>
	awaitable<bool> Client::Pop(std::chrono::steady_clock::duration d, Package<Shared<T>>& ro, std::optional<uint32_t> serverId) {
		if (!*this) co_return false;																// 异常：已断开
		if (peer->waitingObject) co_return false;													// 异常：正在等??
		if (TryPop(ro, serverId)) co_return true;													// 如果已经有数据了，立刻填充 & 返回 true
		peer->waitingObject = true;																	// 标记状态为 正在等
		peer->objectWaiter.expires_after(d);														// 初始化 timer 超时时长
		co_await peer->objectWaiter.async_wait(use_nothrow_awaitable);								// 开始等待
		if (!*this) co_return false;																// 异常：已断开
		co_return TryPop<T>(ro, serverId);															// 立刻填充 & 返回
	}

	template<typename...Args>
	awaitable<bool> Client::WaitOpens(std::chrono::steady_clock::duration d, Args...ids) {
		if (!*this) co_return false;																// 异常：已断开
		if (!peer->openWaitServerIds.empty()) co_return false;										// 异常：正在等??
		(peer->openWaitServerIds.push_back(ids), ...);												// 存储参数
		if (peer->CheckOpens()) co_return true;														// 已 open: 返回 true
		peer->openWaiter.expires_after(d);															// 初始化 timer 超时时长
		co_await peer->openWaiter.async_wait(use_nothrow_awaitable);								// 开始等待
		if (!*this) co_return false;																// 异常：已断开
		if (peer->openWaitServerIds.empty()) co_return true;										// 如果 cmd open 那边 CheckOpens true 会清空参数, 返回 true
		peer->openWaitServerIds.clear();															// 超时, 清空参数, 不再等待. 返回 false
		co_return false;
	}

	template<typename PKG, typename ... Args>
	void Client::SendResponseTo(uint32_t const& target, int32_t const& serial, Args const& ... args) {
		if (!*this) return;
		peer->SendResponseTo<PKG>(target, serial, args...);
	}

	template<typename PKG, typename ... Args>
	void Client::SendPushTo(uint32_t const& target, Args const& ... args) {
		if (!*this) return;
		peer->SendPushTo<PKG>(target, args...);
	}

	template<typename PKG, typename ... Args>
	awaitable<ObjBase_s> Client::SendRequestTo(uint32_t const& target, std::chrono::steady_clock::duration d, Args const& ... args) {
		if (!*this) co_return nullptr;
		co_return co_await peer->SendRequestTo<PKG>(target, d, args...);
	}

	inline void Client::SendTo(uint32_t const& target, int32_t const& serial, Span const& d) {
		if (!*this) return;
		peer->Send(MakeData<true, true, 8192, Span>(*(ObjManager*)1, target, serial, d));
	}
	inline void Client::Send(Data && d) {
		if (!*this) return;
		peer->Send(std::move(d));
	}


	template<typename PKG, typename ... Args>
	void Client::SendResponse(int32_t const& serial, Args const& ... args) {
		assert(cppServerId != 0xFFFFFFFF);
		SendResponseTo<PKG>(cppServerId, serial, args...);
	}

	template<typename PKG, typename ... Args>
	void Client::SendPush(Args const& ... args) {
		assert(cppServerId != 0xFFFFFFFF);
		SendPushTo<PKG>(cppServerId, args...);
	}

	template<typename PKG, typename ... Args>
	awaitable<ObjBase_s> Client::SendRequest(Args const& ... args) {
		assert(cppServerId != 0xFFFFFFFF);
		co_return co_await SendRequestTo<PKG>(cppServerId, 15s, args...);
	}
}
