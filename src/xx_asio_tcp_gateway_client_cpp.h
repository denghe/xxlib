#pragma once
#include <xx_asio_codes.h>

// 为 cpp 实现一个带 拨号 与 通信 的 tcp 网关 协程 client, 基于帧逻辑, 且代码无法 继承扩展, 或设置回调啥的
// todo
// 初期纯 cpp. 在 xx_asio_tcp_client_cpp.h 的基础之上，增加 gateway 的相关支持. 增加完整的 SendXxxxxx 支持
// 后期再增加兼容 lua 代码。
// 将最基础的 Send( Data 映射到 lua, 以实现 lua 端的 发送
// 可先行配置为 当遇到 某些 server id 范围时，数据路由到 lua 或 cpp ( 取决于 游戏类型 ). 一定要及时的，提前的配置( 在收到目标服务的包之前 )
// 路由到 lua 的 所有类型 数据，以 serverId, serial, Data 的格式放入队列。
// 路由到 cpp 的 Push, Request 数据，以 serverId, serial, Obj 的格式放入队列。
// 路由到 cpp 的 Response 数据，直接触发 协程放行
// lua 端可通过映射函数 拉取上述队列的东西，并进一步反序列化投递或协程放行( 均发生在 lua )

namespace xx::Asio::Tcp::Gateway::Cpp {

	struct ReceivedData;
	struct CPeer;
	struct Client : xx::IOCCode<Client> {
		xx::ObjManager om;
		std::shared_ptr<CPeer> peer;																// 当前 peer
		std::unordered_set<uint32_t> cppServerIds;													// 属于 cpp 处理的 serverId 存放于此

		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充

		void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口

		void AddCppServerId(uint32_t id) {															// 添加一个 cpp 服务 id 到 容器
			cppServerIds.insert(id);
		}

		void Update();																				// 每帧开始和逻辑结束时 call 一次

		void Reset();																				// 老 peer Stop + 新建 peer

		operator bool() const;																		// 检测连接有效性. peer && peer->Alive()

		awaitable<int> Dial();																		// 拨号. 成功连上返回 0

		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendResponse(Args const& ... args);													//

		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendPush(Args const& ... args);														//

		//template<typename PKG = xx::ObjBase, typename ... Args>
		//void SendRequest(Args const& ... args);													//

		//bool IsOpened(int serviceId)
	};

	// 收到的数据容器( 含 lua 所有, cpp push + request 类 )
	struct ReceivedData {
		ReceivedData() = default;
		ReceivedData(ReceivedData const&) = default;
		ReceivedData& operator=(ReceivedData const&) = default;
		ReceivedData(ReceivedData&& o) = default;
		ReceivedData& operator=(ReceivedData&& o) = default;

		uint32_t serverId = 0;
		int serial = 0;
		Data data;

		ReceivedData(uint32_t const& serverId, int const& serial, Data&& data)
			: serverId(serverId), serial(serial), data(std::move(data)) {}
	};

	struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer, true>, std::enable_shared_from_this<CPeer> {
		Client& client;
		CPeer(Client& client_)
			: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
			, PeerTimeoutCode(client_.ioc)
			, PeerRequestCode(client_.om)
			, client(client_)
		{
			ResetTimeout(15s);
		}

		std::deque<ReceivedData> recvDatas;														// 已收到的 lua( all ) 数据包集合
		std::deque<std::pair<int, xx::ObjBase_s>> recvObjs;										// 已收到的 cpp(request, push) 数据包集合

		void Stop_() {
			//recvs.clear();
		}

		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
			if (target != 0xFFFFFFFF) return 1;	// passthrough
			// ...
			return 0;
		}

		int ReceiveTargetRequest(uint32_t target, int32_t serial, xx::ObjBase_s&& o_) {
			ResetTimeout(15s);
			// todo: handle o_
			om.KillRecursive(o_);
			return 0;
		}

		int ReceiveTargetPush(uint32_t target, xx::ObjBase_s && o_) {
			ResetTimeout(15s);
			//recvs.emplace_back(target,)
			//recvs.push_back(std::move(o_));													// 数据无脑往 recvs 放
			return 0;
		}

		//int ReceivePush(tartet, xx::ObjBase_s&& o_) {
		//	ResetTimeout(15s);
		//	recvs.push_back(std::move(o_));														// 数据无脑往 recvs 放
		//	if (waitTypeId == -1) return 0;
		//	else if (waitTypeId == 0) {															// WaitPopPackage<xx::ObjBase>
		//		waitBlocker.cancel();
		//	}
		//	else {																				// WaitPopPackage<T>
		//		if (recvs.front().typeId() == waitTypeId) {
		//			waitBlocker.cancel();
		//		}
		//	}
		//	waitTypeId = -1;																	// cleanup
		//	return 0;
		//}
	};

	//inline xx::Shared<T> Client::TryPopPackage() {
	//	if (!check || *this && !peer->recvs.empty()) {
	//		auto r = std::move(peer->recvs.front());
	//		peer->recvs.pop_front();
	//		if constexpr (std::is_same_v<T, xx::ObjBase>) return r;
	//		else if (r.typeId() == xx::TypeId_v<T>) return r.ReinterpretCast<T>();
	//	}
	//	return {};
	//}

	//template<typename T>
	//awaitable<xx::Shared<T>> Client::WaitPopPackage(std::chrono::steady_clock::duration d) {
	//	if (*this && !peer->recvs.empty()) {
	//		co_return TryPopPackage<T, false>();
	//	}
	//	else {
	//		peer->waitBlocker.expires_after(d);
	//		if constexpr (std::is_same_v<T, xx::ObjBase>) {
	//			peer->waitTypeId = 0;
	//		}
	//		else {
	//			peer->waitTypeId = xx::TypeId_v<T>;
	//		}
	//		co_await peer->waitBlocker.async_wait(use_nothrow_awaitable);
	//		co_return TryPopPackage<T, false>();
	//	}
	//}

	//inline void Client::SetDomainPort(std::string_view domain_, uint16_t port_) {
	//	domain = domain_;
	//	port = port_;
	//}

	//inline void Client::Update() {
	//	ioc.poll_one();
	//}

	//template<typename PKG, typename ... Args>
	//inline void Client::Send(Args const& ... args) {
	//	if (!peer || !peer->Alive()) return;
	//	peer->SendPush<PKG>(args...);																// 转发到 peer 的 SendPush 函数
	//}

	//inline Client::operator bool() const {
	//	return peer && peer->Alive();
	//}

	//inline void Client::Reset() {
	//	if (peer) {
	//		peer->Stop();
	//	}
	//	peer = std::make_shared<CPeer>(*this);														// 每次新建 容易控制 协程生命周期, 不容易出问题
	//}

	//inline awaitable<int> Client::Dial() {
	//	if (domain.empty()) co_return __LINE__;
	//	if (!port) co_return __LINE__;
	//	Reset();
	//	asio::ip::address addr;
	//	do {
	//		asio::ip::tcp::resolver resolver(ioc);													// 创建一个域名解析器
	//		auto rr = co_await(resolver.async_resolve(domain, ""sv, use_nothrow_awaitable) || xx::Timeout(5s));	// 开始解析, 得到一个 variant 包含两个协程返回值
	//		if (rr.index() == 1) co_return __LINE__;												// 如果 variant 存放的是第 2 个结果, 那就是 超时
	//		auto& [e, rs] = std::get<0>(rr);														// 展开第一个结果为 err + results
	//		if (e) co_return __LINE__;																// 出错: 解析失败
	//		auto iter = rs.cbegin();																// 拿到迭代器，指向首个地址
	//		if (auto idx = (int)((size_t)xx::NowEpochMilliseconds() % rs.size())) {					// 根据当前 ms 时间点 随机选一个下标
	//			std::advance(iter, idx);															// 快进迭代器
	//		}
	//		addr = iter->endpoint().address();														// 从迭代器获取地址
	//	} while (false);
	//	if (auto r = co_await peer->Connect(addr, port, 5s)) co_return __LINE__;					// 开始连接. 超时 5 秒
	//	co_return 0;
	//}

}


// for lua

// AddCppServiceId
// int SendEcho(xx::Data data);
// int SendTo(int serviceId, int serial, xx::Data data) 
