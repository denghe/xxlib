#pragma once
#include <xx_asio_tcp_base.h>
#include <xx_asio_udp_base.h>
#include <xx_asio_gateway_helpers.h>
#include <ikcp.h>

#ifdef _DEBUG
#define XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT 0
#endif // _DEBUG

// 为 cpp 实现一个带 拨号 与 通信 的 tcp/udp/kcp 网关 协程 client, 基于帧逻辑, 无需继承, 开箱即用

namespace xx::Asio::Gateway {

	struct TPeer;	// tcp peer
	struct KPeer;	// kcp peer

	struct Client : xx::IOCBase {
		typedef asio::io_context::executor_type ExecutorType;
		asio::executor_work_guard<ExecutorType> work_guard_;
		Client() : work_guard_(asio::make_work_guard(*this)), openWaiter(*this), objectWaiter(*this) {}

		bool isTcp = true;																			// tpeer ? kpeer ?
		xx::Shared<TPeer> tpeer;
		xx::Shared<KPeer> kpeer;

		uint32_t cppServerId = 0xFFFFFFFF;															// 属于 cpp 处理的 serverId 存放于此

		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充
		bool busy = false;																			// 繁忙标志 for lua ( lua bind 函数在 co_spawn 之前 check & set, 之后 clear )
		int dialResult = 0;																			// 拨号结果，方便lua读取

		xx::Data secret, recv;

		std::deque<Package<Data>> recvDatas;														// 已收到的 lua( all ) 数据包集合
		std::unordered_map<uint32_t, std::deque<Package<ObjBase_s>>> recvObjs;						// 已收到的 cpp(request, push) 数据包集合. 按照 serverId 分组存储

		std::unordered_set<uint32_t> openServerIds;													// 已 open 的 serverId 存放于此

		asio::steady_timer openWaiter;																// open 等待器
		std::vector<uint32_t> openWaitServerIds;													// 参与等待的 id 列表

		asio::steady_timer objectWaiter;															// 数据包等待器
		bool waitingObject = false;																	// 数据包等待状态

		void Cleanup();																				// cleanup 上面这些容器, 变量

		void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口

		void SetCppServerId(uint32_t id);															// 设置 cpp 服务 id

		void Update();																				// 每帧开始和逻辑结束时 call 一次

		void Reset(bool isTcp = true);																// 老 peer Stop + 新建 peer

		operator bool() const;																		// 检测连接有效性. peer && peer->Alive()

		awaitable<int> Dial(bool isTcp = true, bool enableTcpEncrypt = false);       				// 拨号. 成功连上返回 0. isTcp == false: kcp

		template<typename...Args>
		awaitable<bool> WaitOpens(std::chrono::steady_clock::duration d, Args...ids);				// 带超时同时等一批 server id open. 超时返回 false

		bool IsOpened(uint32_t serverId);															// 检查当前某 serverId 是否已 open

		bool CheckOpens();																			// openWaitServerIds 所有 id 都 open 则返回 true

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
		void Send(Data&& d);																		// 转发到 peer-> 同名函数 ( for lua )

		// for cpp easy use

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponse(int32_t const& serial, Args const& ... args);								// SendResponseTo( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		void SendPush(Args const& ... args);														// SendPush( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequest(Args const& ... args);										// SendRequest( cppServerId, 15s, ...
	};

	struct TPeer : xx::PeerTcpBaseCode<TPeer, Client>, TcpKcpRequestTargetCodes<TPeer, Client> {
		using BT = xx::PeerTcpBaseCode<TPeer, Client>;
		using BT::BT;

		asio::ip::tcp::endpoint serverEP;

		// client: dial & connect to serverEP
		awaitable<int> Connect(std::chrono::steady_clock::duration d = 5s, bool enableEncrypt = true) {
			if (auto r = co_await BT::Connect(serverEP, d)) co_return __LINE__;

			auto& secret = ioc.secret;
			secret.Clear();
			if (enableEncrypt) {
				secret = GenSecret(5, 250);
				assert(secret.len <= 255);

				secret.buf[-1] = (unsigned char)secret.len;
				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(secret.buf - 1, secret.len + 1), use_nothrow_awaitable);
				if (ec) {
					co_return __LINE__;
				}
				if (n != secret.len + 1) {
					this->Stop();
					co_return __LINE__;
				}
			}
			co_return 0;
		}

		void Stop_() {																				// 所有环境变量恢复初始状态, 取消所有等待
			ioc.Cleanup();
		}

		template<typename D>
		void Send(D&& d) {
			auto buf = d.GetBuf();
			auto len = d.GetLen();
			if(ioc.secret.len > 0) xx::XorContent((char*)ioc.secret.buf, ioc.secret.len, (char*)buf + 4, len - 4);	// 4: skip len
			BT::Send(std::forward<D>(d));
		}

		awaitable<void> Read(auto memHolder) {
			auto& recv = ioc.recv;
			recv.Clear();
			recv.Reserve(1024 * 1024);

			for (;;) {
				auto [ec, n] = co_await socket.async_read_some(asio::buffer(recv.buf + recv.len, recv.cap - recv.len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (!n) continue;
				recv.len += n;

				size_t offset = 0;
				while (offset + 4 <= recv.len) {						// ensure header len( 4 bytes )
					uint32_t len = recv[offset + 0] + (recv[offset + 1] << 8) + (recv[offset + 2] << 16) + (recv[offset + 3] << 24);
					if (offset + 4 + len > recv.len) break;				// not enough data
					offset += 4;

					Data_r dr{ recv.buf + offset, len };
					if(ioc.secret.len > 0) xx::XorContent((char*)ioc.secret.buf, ioc.secret.len, (char*)dr.buf, dr.len);
					if (int r = this->HandleData(std::move(dr))) {
						Stop();
						co_return;
					}
					offset += len;
				}
				recv.RemoveFront(offset);
			}
			Stop();
		}
	};

	struct KPeer : xx::PeerUdpBaseCode<KPeer, Client>, TcpKcpRequestTargetCodes<KPeer, Client> {
		using BT = xx::PeerUdpBaseCode<KPeer, Client>;
		using BT::BT;

		asio::ip::udp::endpoint serverEP;

		ikcpcb* kcp = nullptr;
		uint32_t conv = 0;
		int64_t createMS = 0;
		uint32_t nextUpdateMS = 0;
		bool busy = false;

		void InitKcp() {
			assert(!kcp);
			kcp = ikcp_create(conv, this);
			(void)ikcp_wndsize(kcp, 1024, 1024);
			(void)ikcp_nodelay(kcp, 1, 10, 2, 1);
			kcp->rx_minrto = 10;
			kcp->stream = 1;
			//ikcp_setmtu(kcp, 470);    // maybe speed up because small package first role
			createMS = xx::NowSteadyEpochMilliseconds();

			ikcp_setoutput(kcp, [](const char* inBuf, int len, ikcpcb* _, void* user) -> int {

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				xx::Cout("\n ikcp output = ", xx::Data_r(inBuf, len));
#endif
				auto self = (KPeer*)user;
				auto& secret = self->ioc.secret;

				xx::Data d;
				d.Resize(len);
				d.WriteBufAt(0, inBuf, 4);	// copy convId without encrypt
				size_t j = 0;
				for (size_t i = 4; i < len; i++) {		// 4: skip convId
					d[i] = inBuf[i] ^ secret[j++];
					if (j == secret.len) {
						j = 0;
					}
				}

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				xx::Cout("\n send to = ", d);
#endif
				self->SendTo(self->serverEP, std::move(d));
				return 0;
				});

			co_spawn(ioc, Update(xx::SharedFromThis(this)), detached);
		}

		// client: dial & connect to serverEP
		awaitable<int> Connect(std::chrono::steady_clock::duration d = 5s) {
			if (!stoped) co_return 1;
			assert(!kcp);
			assert(!busy);
			busy = true;
			auto busySG = xx::MakeSimpleScopeGuard([this] {busy = false; });
			xx::Data s;
			try {
				asio::steady_timer timer(ioc);
				timer.expires_after(d);
				timer.async_wait([this](auto const& e) {
					if (e) return;
					boost::system::error_code ignore;
					socket.close(ignore);
					});

				socket = asio::ip::udp::socket(ioc);    // for macos
				co_await socket.async_connect(serverEP, asio::use_awaitable);

				auto& secret = ioc.secret;
				secret = GenSecret();
				co_await socket.async_send(asio::buffer(secret.buf, secret.len), asio::use_awaitable);

				s.Resize(secret.len + 4);
				auto rc = co_await socket.async_receive(asio::buffer(s.buf, s.len), asio::use_awaitable);
				if (rc != s.len) {
					co_return 3;
				}
				if (memcmp(secret.buf, s.buf + 4, secret.len)) co_return 4;
				memcpy(&conv, s.buf, 4);

			} catch (...) {
				co_return 2;
			}
			InitKcp();
			Start();

			s.Clear();
			s.Fill({ 1,0,0,0,0 });
			busy = false;
			Send(std::move(s));

			co_return 0;
		}

		void Stop_() {																				// 所有环境变量恢复初始状态, 取消所有等待
			assert(kcp);
			assert(!busy);
			ikcp_release(kcp);
			kcp = nullptr;
			ioc.Cleanup();
		}

		// client: send to serverEP
		template<typename D>
		void Send(D&& d) {
			assert(kcp);
			assert(!busy);
			assert(serverEP != asio::ip::udp::endpoint{});
			// 据kcp文档讲, 只要在等待期间发生了 ikcp_send, ikcp_input 就要重新 update
			nextUpdateMS = 0;
			int r = ikcp_send(kcp, (char*)d.GetBuf(), (int)d.GetLen());
			assert(!r);
			ikcp_flush(kcp);
		}

		awaitable<void> Read(auto memHolder) {
			auto buf = std::make_unique<uint8_t[]>(1024 * 1024);
			auto& recv = ioc.recv;
			recv.Clear();

			for (;;) {
				asio::ip::udp::endpoint ep;	// unused here
				auto [ec, len] = co_await socket.async_receive_from(asio::buffer(buf.get(), 1024 * 1024), ep, use_nothrow_awaitable);
				if (ec) {
#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
					std::cout << "\nRead ec = " << ec << std::endl;
#endif
					break;
				}
				if (stoped) co_return;
				if (!len) continue;

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				xx::CoutN("\nasync_receive_from. buf data = ", xx::Data_r(buf.get(), len));
#endif

				// decrypt
				xx::XorContent((char*)ioc.secret.buf, ioc.secret.len, (char*)buf.get() + 4, len - 4);	// 4: skip convId

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
				xx::CoutN("async_receive_from. decrypted buf data = ", xx::Data_r(buf.get(), len));
#endif

				// put raw data into kcp
				if (int r = ikcp_input(kcp, (char*)buf.get(), (long)len)) {
					std::cout << "\nikcp_input error. r = " << r << std::endl;
					co_return;
				}

				// 据kcp文档讲, 只要在等待期间发生了 ikcp_send, ikcp_input 就要重新 update
				nextUpdateMS = 0;

				// get decrypted data from kcp
				do {
					// 拿内存需求长度
					auto r = ikcp_peeksize(kcp);
					if (r < 0) break;

					// 按需扩容
					auto n = recv.cap - recv.len;
					if (n < r) {
						recv.Reserve(recv.cap + r - n);
					}

					// 从 kcp 提取数据. 追加填充到 recv 后面区域. 返回填充长度. <= 0 则下次再说
					auto&& len = ikcp_recv(kcp, (char*)recv.buf + recv.len, (int)(recv.cap - recv.len));
					if (len <= 0) break;
					recv.len += len;

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
					xx::Cout("\n *********************************  kcp decoded recv = ", recv);
#endif

					// 调用 recv 数据处理函数
					Receive();

					if (IsStoped())
						co_return;
				} while (true);
			}
			Stop();
		}

		// 解析 recv 容器的内容, 切片， call HandleData
		void Receive() {
			auto& recv = ioc.recv;
			size_t offset = 0;
			while (offset + 4 <= recv.len) {						// ensure header len( 4 bytes )
				uint32_t len = recv[offset + 0] + (recv[offset + 1] << 8) + (recv[offset + 2] << 16) + (recv[offset + 3] << 24);
				if (offset + 4 + len > recv.len) break;				// not enough data
				offset += 4;
				if (int r = HandleData(Data_r{ recv.buf + offset, len })) {
					Stop();
					return;
				}
				offset += len;
			}
			recv.RemoveFront(offset);
		}

		awaitable<void> Update(auto memHolder) {
			while (kcp) {
				// 计算出当前 ms
				// 已知问题: 受 ikcp uint32 限制, 连接最多保持 50 多天
				auto&& currentMS = uint32_t(xx::NowSteadyEpochMilliseconds() - createMS);
				// 如果 update 时间没到 就退出
				if (nextUpdateMS <= currentMS) {
					// 来一发
					ikcp_update(kcp, currentMS);

#if XX_ASIO_TCPKCP_DEBUG_INFO_OUTPUT
					std::cout << "!";
#endif

					// 更新下次 update 时间
					nextUpdateMS = ikcp_check(kcp, currentMS);
				}
				// sleep
				co_await xx::Timeout(10ms);
			}
		}

	};


	inline void Client::Cleanup() {
		secret.Clear();
		recv.Clear();
		recvDatas.clear();
		recvObjs.clear();
		openServerIds.clear();
		openWaitServerIds.clear();
		waitingObject = false;
		openWaiter.cancel();
		objectWaiter.cancel();
	}

	inline void Client::SetCppServerId(uint32_t id) {
		cppServerId = id;
	}

	inline void Client::SetDomainPort(std::string_view domain_, uint16_t port_) {
		domain = domain_;
		port = port_;
	}

	inline void Client::Update() {
		poll_one();
	}

	inline Client::operator bool() const {
		if (isTcp) {
			return tpeer && !tpeer->IsStoped();
		}
		else {
			return kpeer && !kpeer->IsStoped();
		}
	}

	inline void Client::Reset(bool isTcp) {															// 每次新建 容易控制 协程生命周期, 不容易出问题
		if (tpeer) {
			tpeer->Stop();
			tpeer.Reset();
		}
		if (kpeer) {
			kpeer->Stop();
			kpeer.Reset();
		}
		if (isTcp) {
			tpeer = xx::Make<TPeer>(*this);
		}
		else {
			kpeer = xx::Make<KPeer>(*this);
		}
		this->isTcp = isTcp;
	}

	inline awaitable<int> Client::Dial(bool isTcp, bool enableTcpEncrypt) {
		if (domain.empty()) co_return __LINE__;
		if (!port) co_return __LINE__;
		Reset(isTcp);
		asio::ip::address addr;
		do {
			asio::ip::udp::resolver resolver(*this);												// 创建一个域名解析器
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
		if (isTcp) {
			tpeer->serverEP = { addr, port };
			if (auto r = co_await tpeer->Connect(5s, enableTcpEncrypt)) co_return __LINE__;			// 开始连接. 超时 5 秒
		} else {
			kpeer->serverEP = { addr, port };
			if (auto r = co_await kpeer->Connect(5s)) co_return __LINE__;							// 开始连接. 超时 5 秒
		}
		co_return 0;
	}

	inline bool Client::IsOpened(uint32_t serverId) {
		if (!*this) return false;
		if (openServerIds.empty()) return false;
		return openServerIds.contains(serverId);
	}

	inline bool Client::CheckOpens() {
		if (!*this) return false;
		for (auto& id : openWaitServerIds) {
			if (!openServerIds.contains(id)) return false;
		}
		openWaitServerIds.clear();															// 成功后清空参数
		return true;
	}

	inline bool Client::TryPop(Package<Data>& rd) {
		if (!*this) return false;
		if (recvDatas.empty()) return false;
		rd = std::move(recvDatas.front());
		recvDatas.pop_front();
		return true;
	}

	template<typename T>
	bool Client::TryPop(Package<Shared<T>>& ro, std::optional<uint32_t> serverId) {
		if (!*this) return false;																	// 异常：已断开
		std::deque<Package<ObjBase_s>>* os = nullptr;
		if (serverId.has_value()) {																	// 如果有指定 serverId
			os = &recvObjs[serverId.value()];														// 定位到队列
			if (os->empty()) return false;															// 空: 短路出去
		} else {
			for (auto& kv : recvObjs) {																// 随便找出一个有数据的队列
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
		if (waitingObject) co_return false;															// 异常：正在等??
		if (TryPop(ro, serverId)) co_return true;													// 如果已经有数据了，立刻填充 & 返回 true
		waitingObject = true;																		// 标记状态为 正在等
		objectWaiter.expires_after(d);																// 初始化 timer 超时时长
		co_await objectWaiter.async_wait(use_nothrow_awaitable);									// 开始等待
		if (!*this) co_return false;																// 异常：已断开
		co_return TryPop<T>(ro, serverId);															// 立刻填充 & 返回
	}

	template<typename...Args>
	awaitable<bool> Client::WaitOpens(std::chrono::steady_clock::duration d, Args...ids) {
		if (!*this) co_return false;																// 异常：已断开
		if (!openWaitServerIds.empty()) co_return false;											// 异常：正在等??
		(openWaitServerIds.push_back(ids), ...);													// 存储参数
		if (CheckOpens()) co_return true;															// 已 open: 返回 true
		openWaiter.expires_after(d);																// 初始化 timer 超时时长
		co_await openWaiter.async_wait(use_nothrow_awaitable);										// 开始等待
		if (!*this) co_return false;																// 异常：已断开
		if (openWaitServerIds.empty()) co_return true;												// 如果 cmd open 那边 CheckOpens true 会清空参数, 返回 true
		openWaitServerIds.clear();																	// 超时, 清空参数, 不再等待. 返回 false
		co_return false;
	}

	template<typename PKG, typename ... Args>
	void Client::SendResponseTo(uint32_t const& target, int32_t const& serial, Args const& ... args) {
		if (!*this) return;
		if (isTcp) {
			tpeer->SendResponseTo<PKG>(target, serial, args...);
		} else {
			kpeer->SendResponseTo<PKG>(target, serial, args...);
		}
	}

	template<typename PKG, typename ... Args>
	void Client::SendPushTo(uint32_t const& target, Args const& ... args) {
		if (!*this) return;
		if (isTcp) {
			tpeer->SendPushTo<PKG>(target, args...);
		} else {
			kpeer->SendPushTo<PKG>(target, args...);
		}
	}

	template<typename PKG, typename ... Args>
	awaitable<ObjBase_s> Client::SendRequestTo(uint32_t const& target, std::chrono::steady_clock::duration d, Args const& ... args) {
		if (!*this) co_return nullptr;
		if (isTcp) {
			co_return co_await tpeer->SendRequestTo<PKG>(target, d, args...);
		} else {
			co_return co_await kpeer->SendRequestTo<PKG>(target, d, args...);
		}
	}

	inline void Client::SendTo(uint32_t const& target, int32_t const& serial, Span const& dr) {
		if (!*this) return;
		if (isTcp) {
			tpeer->Send(MakeData<true, true, 8192, Span>(*(ObjManager*)1, target, serial, dr));
		} else {
			kpeer->Send(MakeData<true, true, 8192, Span>(*(ObjManager*)1, target, serial, dr));
		}
	}
	inline void Client::Send(Data&& d) {
		if (!*this) return;
		if (isTcp) {
			tpeer->Send(std::move(d));
		} else {
			kpeer->Send(std::move(d));
		}
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
