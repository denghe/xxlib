#pragma once
#include <xx_asio_codes.h>

// 为 cpp 实现一个带 拨号 与 通信 的 tcp 协程 client, 基于帧逻辑, 无需继承, 开箱即用( 从 网关版 删改而来 )

namespace xx::Asio::Tcp {

	// 收到的数据容器 T == Data: for script     T == xx::ObjBase_s for c++
	template<typename T>
	struct Package {
		XX_OBJ_STRUCT_H(Package);
		int32_t serial = 0;
		T data;
		Package(int32_t const& serial, T&& data) : serial(serial), data(std::move(data)) {}
	};

	struct CPeer;
	struct Client : IOCCode<Client> {
		ObjManager om;
		std::shared_ptr<CPeer> peer;																// 当前 peer

		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充

		void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口

		void Update();																				// 每帧开始和逻辑结束时 call 一次

		void Reset();																				// 老 peer Stop + 新建 peer

		operator bool() const;																		// 检测连接有效性. peer && peer->Alive()

		awaitable<int> Dial();																		// 拨号. 成功连上返回 0

		template<typename T = ObjBase>
		bool TryPop(Package<Shared<T>>& ro);														// 试获取一条消息内容. 没有则返回 false. 类型不符 填 null

		bool TryPop(Package<Data>& rd);																// 试获取一条消息数据. 没有则返回 false

		template<typename T = ObjBase>
		awaitable<bool> Pop(std::chrono::steady_clock::duration d, Package<Shared<T>>& ro);			// 带超时 等待 接收一条消息. 超时返回 false. 类型不符也返回 false

		template<typename T = ObjBase>
		awaitable<Shared<T>> PopPush(std::chrono::steady_clock::duration d);						// Pop 的 直接返回 Push 值版本. 如果 serial != 0 也返回 null

		template<typename T = ObjBase>
		Shared<T> TryPopPush();																		// 试获取一条 Push. 没有则返回 null. 类型不符 返回 null

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponse(int32_t const& serial, Args const& ... args);								// 转发到 peer-> 同名函数

		template<typename PKG = ObjBase, typename ... Args>
		void SendPush(Args const& ... args);														// 转发到 peer-> 同名函数

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequest(std::chrono::steady_clock::duration d, Args const& ... args);	// 转发到 peer-> 同名函数

		void Send(Data&& d);																		// 转发到 peer-> 同名函数 ( for lua )
	};

	struct CPeer : PeerCode<CPeer>, PeerTimeoutCode<CPeer>, PeerRequestCode<CPeer>, std::enable_shared_from_this<CPeer> {
		Client& client;
		CPeer(Client& client_)
			: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
			, PeerTimeoutCode(client_.ioc)
			, PeerRequestCode(client_.om)
			, client(client_)
			, objectWaiter(client_.ioc)
		{
			ResetTimeout(15s);
		}

		std::deque<Package<Data>> recvDatas;														// 已收到的 lua( all ) 数据包集合
		std::deque<Package<ObjBase_s>> recvObjs;													// 已收到的 cpp(request, push) 数据包集合

		asio::steady_timer objectWaiter;															// 数据包等待器
		bool waitingObject = false;																	// 数据包等待状态

		void Stop_() {																				// 所有环境变量恢复初始状态, 取消所有等待
			recvDatas.clear();
			recvObjs.clear();
			waitingObject = false;
			objectWaiter.cancel();																	// 取消某些等待器
		}
		int ReceiveResponse(int32_t, ObjBase_s&) {													// 续命
			ResetTimeout(15s);
			return 0;
		}
		int ReceiveRequest(int32_t serial, ObjBase_s&& o_) {
			ResetTimeout(15s);																		// 续命
			recvObjs.emplace_back(serial, std::move(o_));											// 放入 recvObjs
			if (waitingObject) {																	// 如果发现正在等包
				waitingObject = false;																// 清理标志位
				objectWaiter.cancel();																// 放行
			}
			return 0;
		}
		int ReceivePush(ObjBase_s&& o_) {
			return ReceiveRequest(0, std::move(o_));
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

	template<typename T>
	inline bool Client::TryPop(Package<Shared<T>>& ro) {
		if (!*this || peer->recvObjs.empty()) return false;
		auto& o = peer->recvObjs.front();
		if constexpr (std::is_same_v<T, ObjBase>) {
			ro = std::move(o);
		}
		else {
			static_assert(std::is_base_of_v<ObjBase, T>);
			if (o.data.GetTypeId() == xx::TypeId_v<T>) {
				ro = std::move(reinterpret_cast<Package<Shared<T>>&>(o));
			}
			else {
				ro.serial = 0;
				ro.data.Reset();
			}
		}
		peer->recvObjs.pop_front();
		return true;
	}

	inline bool Client::TryPop(Package<Data>& rd) {
		if (!*this || peer->recvDatas.empty()) return false;
		rd = std::move(peer->recvDatas.front());
		peer->recvDatas.pop_front();
		return true;
	}

	template<typename T>
	awaitable<bool> Client::Pop(std::chrono::steady_clock::duration d, Package<Shared<T>>& ro) {
		if (!*this) co_return false;																// 异常：已断开
		if (peer->waitingObject) co_return false;													// 异常：正在等??
		if (TryPop(ro)) co_return true;																// 如果已经有数据了，立刻填充 & 返回 true
		peer->waitingObject = true;																	// 标记状态为 正在等
		peer->objectWaiter.expires_after(d);														// 初始化 timer 超时时长
		co_await peer->objectWaiter.async_wait(use_nothrow_awaitable);								// 开始等待
		if (!*this) co_return false;																// 异常：已断开
		co_return TryPop<T>(ro);																	// 立刻填充 & 返回
	}

	template<typename T>
	awaitable<Shared<T>> Client::PopPush(std::chrono::steady_clock::duration d) {
		if (!*this) co_return nullptr;
		Package<Shared<T>> ro;
		co_await Pop(d, ro);
		if (ro.serial == 0) co_return std::move(ro.data);
		om.KillRecursive(ro.data);
		co_return nullptr;
	}

	template<typename T>
	Shared<T> Client::TryPopPush() {
		Package<Shared<T>> ro;
		if (!TryPop(ro)) return nullptr;
		if (ro.serial == 0) return std::move(ro.data);
		om.KillRecursive(ro.data);
		return nullptr;
	}

	template<typename PKG, typename ... Args>
	void Client::SendResponse(int32_t const& serial, Args const& ... args) {
		if (!*this) return;
		peer->SendResponse<PKG>(serial, args...);
	}

	template<typename PKG, typename ... Args>
	void Client::SendPush(Args const& ... args) {
		if (!*this) return;
		peer->SendPush<PKG>(args...);
	}

	template<typename PKG, typename ... Args>
	awaitable<ObjBase_s> Client::SendRequest(std::chrono::steady_clock::duration d, Args const& ... args) {
		if (!*this) co_return nullptr;
		co_return co_await peer->SendRequest<PKG>(d, args...);
	}

	inline void Client::Send(Data&& d) {
		if (!*this) return;
		peer->Send(std::move(d));
	}
}
