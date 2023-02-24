#pragma once
#include <xx_asio_udp_base.h>
#include <xx_asio_request.h>
#include <random>
#include <ikcp.h>
#include <xx_obj.h>

// 为 cpp 实现一个带 拨号 与 通信 的 udp/kcp 网关 协程 client, 基于帧逻辑, 无需继承, 开箱即用

namespace xx::Asio::Kcp::Gateway {

	// 收到的数据容器 T == Data: for script     T == xx::ObjBase_s for c++
	template<typename T>
	struct Package {
		XX_OBJ_STRUCT_H(Package);
		uint32_t serverId = 0;
		int32_t serial = 0;
		T data;
		Package(uint32_t const& serverId, int32_t const& serial, T&& data) : serverId(serverId), serial(serial), data(std::move(data)) {}
	};

	// 构造一个 uint32_le 数据长度( 不含自身 ) + uint32_le target + args 的 类 序列化包 并返回
	template<bool containTarget, bool containSerial, size_t cap, typename PKG = ObjBase, typename ... Args>
	Data MakeData(ObjManager& om, uint32_t const& target, int32_t const& serial, Args const&... args) {
		Data d;
		d.Reserve(cap);
		auto bak = d.WriteJump<false>(sizeof(uint32_t));
		if constexpr (containTarget) {
			d.WriteFixed<false>(target);
		}
		if constexpr (containSerial) {
			d.WriteVarInteger<false>(serial);
		}
		// 传统写包
		if constexpr (std::is_same_v<ObjBase, PKG>) {
			om.WriteTo(d, args...);
		}
		// 直写 cache buf 包
		else if constexpr (std::is_same_v<Span, PKG>) {
			d.WriteBufSpans(args...);
		}
		// 简单构造命令行包
		else if constexpr (std::is_same_v<std::string, PKG>) {
			d.Write(args...);
		}
		// 使用 目标类的静态函数 快速填充 buf
		else {
			PKG::WriteTo(d, args...);
		}
		d.WriteFixedAt(bak, (uint32_t)(d.len - 4));
		return d;
	}
	// 构造一个 uint32_le 数据长度( 不含自身 ) + args 的 类 序列化包 并返回
	template<size_t cap = 8192, typename PKG = ObjBase, typename ... Args>
	Data MakeTargetPackageData(ObjManager& om, uint32_t const& target, int32_t const& serial, Args const&... args) {
		return MakeData<true, true, cap, PKG>(om, target, serial, args...);
	}


	struct CPeer;
	struct Client : xx::IOCBase {
		typedef asio::io_context::executor_type ExecutorType;
		asio::executor_work_guard<ExecutorType> work_guard_;
		Client() : work_guard_(asio::make_work_guard(*this)) {}

		std::shared_ptr<CPeer> peer;																// 当前 peer
		uint32_t cppServerId = 0xFFFFFFFF;															// 属于 cpp 处理的 serverId 存放于此

		std::string secret;																			// 用于数据滚动异或
		std::string domain;																			// 域名/ip. 需要在 拨号 前填充
		uint16_t port = 0;																			// 端口. 需要在 拨号 前填充
		bool busy = false;																			// 繁忙标志 for lua ( lua bind 函数在 co_spawn 之前 check & set, 之后 clear )

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
		void Send(Data&& d);																		// 转发到 peer-> 同名函数 ( for lua )

		// for cpp easy use

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponse(int32_t const& serial, Args const& ... args);								// SendResponseTo( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		void SendPush(Args const& ... args);														// SendPush( cppServerId, ...

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequest(Args const& ... args);										// SendRequest( cppServerId, 15s, ...
	};

	struct CPeer : xx::PeerUdpBaseCode<CPeer, Client> {
		using BT = xx::PeerUdpBaseCode<CPeer, Client>;
		CPeer(Client& ioc_)
			: BT(ioc_)
			, openWaiter(ioc_)
			, objectWaiter(ioc_)
		{
		}

		asio::ip::udp::endpoint serverEP;
		xx::Data secret, recv;
		ikcpcb* kcp = nullptr;
		uint32_t conv = 0;
		int64_t createMS = 0;
		uint32_t nextUpdateMS = 0;
		bool busy = false;


		std::deque<Package<Data>> recvDatas;														// 已收到的 lua( all ) 数据包集合
		std::unordered_map<uint32_t, std::deque<Package<ObjBase_s>>> recvObjs;						// 已收到的 cpp(request, push) 数据包集合. 按照 serverId 分组存储

		std::unordered_set<uint32_t> openServerIds;													// 已 open 的 serverId 存放于此

		asio::steady_timer openWaiter;																// open 等待器
		std::vector<uint32_t> openWaitServerIds;													// 参与等待的 id 列表

		asio::steady_timer objectWaiter;															// 数据包等待器
		bool waitingObject = false;																	// 数据包等待状态

	protected:
		void FillSecret() {
			std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<int> dis1(5, 19);	// 4: old protocol
			std::uniform_int_distribution<int> dis2(0, 255);
			auto siz = dis1(rng);
			secret.Resize(siz);
			for (size_t i = 0; i < siz; i++) {
				secret[i] = dis2(rng);
			}
		}

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

				xx::Cout("\n ikcp output = ", xx::Data_r(inBuf, len));

				auto self = (CPeer*)user;
				xx::Data d;
				d.Resize(len);
				d.WriteBufAt(0, inBuf, 4);	// copy convId without encrypt
				size_t j = 0;
				for (size_t i = 4; i < len; i++) {		// 4: skip convId
					d[i] = inBuf[i] ^ self->secret[j++];
					if (j == self->secret.len) {
						j = 0;
					}
				}

				xx::Cout("\n send to = ", d);
				self->SendTo(self->serverEP, std::move(d));
				return 0;
				});

			co_spawn(ioc, Update(xx::SharedFromThis(this)), detached);
		}
	public:

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

				FillSecret();
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

			recvDatas.clear();
			recvObjs.clear();
			openServerIds.clear();
			openWaitServerIds.clear();
			waitingObject = false;
			openWaiter.cancel();
			objectWaiter.cancel();
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
			for (;;) {
				asio::ip::udp::endpoint ep;	// unused here
				auto [ec, len] = co_await socket.async_receive_from(asio::buffer(buf.get(), 1024 * 1024), ep, use_nothrow_awaitable);
				if (ec) {
					std::cout << "\nRead ec = " << ec << std::endl;
					break;
				}
				if (stoped) co_return;
				if (!len) continue;

				xx::CoutN("\nasync_receive_from. buf data = ", xx::Data_r(buf.get(), len));

				// decrypt
				size_t j = 0;
				for (size_t i = 4; i < len; i++) {		// 4: skip convId
					buf[i] = buf[i] ^ secret[j++];
					if (j == secret.len) {
						j = 0;
					}
				}

				xx::CoutN("async_receive_from. decrypted buf data = ", xx::Data_r(buf.get(), len));

				// put raw data into kcp
				if (int r = ikcp_input(kcp, (char*)buf.get(), (long)len)) {
					std::cout << "\nikcp_input error. r = " << r << std::endl;
					co_return;
				}

				// 据kcp文档讲, 只要在等待期间发生了 ikcp_send, ikcp_input 就要重新 update
				nextUpdateMS = 0;

				// get decrypted data from kcp
				do {
					// 如果接收缓存没容量就扩容( 通常发生在首次使用时 )
					if (!recv.cap) {
						recv.Reserve(1024 * 256);
					}
					// 如果数据长度 == buf限长 就自杀( 未处理数据累计太多? )
					if (recv.len == recv.cap) goto LabStop;

					// 从 kcp 提取数据. 追加填充到 recv 后面区域. 返回填充长度. <= 0 则下次再说
					auto&& len = ikcp_recv(kcp, (char*)recv.buf + recv.len, (int)(recv.cap - recv.len));
					if (len <= 0) break;
					recv.len += len;

					xx::Cout("\nkcp decoded recv = ", recv);

					// 调用 recv 数据处理函数
					Receive();

					if (IsStoped()) co_return;
				} while (true);
			}
		LabStop:
			Stop();
		}

		// 解析 recv 容器的内容, 切片， call HandleData
		void Receive() {
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

					std::cout << "!";

					// 更新下次 update 时间
					nextUpdateMS = ikcp_check(kcp, currentMS);
				}
				// sleep
				co_await xx::Timeout(10ms);
			}
		}


		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, Data_r& dr) {
			if (target == 0xFFFFFFFF) return ReceveCommand(dr);										// 收到 command. 转过去处理
			if (ioc.cppServerId == target) return 1; 												// 属于 cpp 的: passthrough
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

				std::cout << "\nrecv cmd: open " << serviceId << std::endl;

				openServerIds.insert(serviceId);													// 添加到 open列表
				if (!openWaitServerIds.empty()) {													// 判断是否正在 等open
					if (CheckOpens()) {																// 全 open 了: 清除参数 & cancel 等待器
						openWaiter.cancel();
					}
				}
			} else if (cmd == "close"sv) {															// 收到 关闭服务 指令
				uint32_t serviceId = -1;
				if (dr.Read(serviceId)) return -__LINE__;

				std::cout << "\nrecv cmd: close " << serviceId << std::endl;

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
		int ReceiveTargetPush(uint32_t target, ObjBase_s&& o_) {
			return ReceiveTargetRequest(target, 0, std::move(o_));
		}

		bool CheckOpens() {																			// openWaitServerIds 所有 id 都 open 则返回 true
			for (auto& id : openWaitServerIds) {
				if (!openServerIds.contains(id)) return false;
			}
			openWaitServerIds.clear();																// 成功后清空参数
			return true;
		}



		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;
		ObjManager om;

		template<typename PKG = ObjBase, typename ... Args>
		void SendResponseTo(uint32_t const& target, int32_t const& serial, Args const& ... args) {
			Send(MakeTargetPackageData<8192, PKG>(om, target, serial, args...));
		}

		template<typename PKG = ObjBase, typename ... Args>
		void SendPushTo(uint32_t const& target, Args const& ... args) {
			Send(MakeTargetPackageData<8192, PKG>(om, target, 0, args...));
		}

		template<typename PKG = ObjBase, typename ... Args>
		awaitable<ObjBase_s> SendRequestTo(uint32_t const& target, std::chrono::steady_clock::duration d, Args const& ... args) {
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto key = reqAutoId;
			auto [iter, ok] = reqs.emplace(key, std::make_pair(asio::steady_timer(ioc, std::chrono::steady_clock::now() + d), ObjBase_s()));
			assert(ok);
			auto& [timer, data] = iter->second;
			Send(MakeTargetPackageData<8192, PKG>(om, target, -key, args...));
			auto [e] = co_await timer.async_wait(use_nothrow_awaitable);
			if (stoped || (e && (iter = reqs.find(key)) == reqs.end())) co_return nullptr;
			auto r = std::move(data);
			reqs.erase(iter);
			co_return r;
		}

		ObjBase_s ReadFrom(Data_r& dr) {
			ObjBase_s o;
			int r = om.ReadFrom(dr, o);
			if (r || (o && o.GetTypeId() == 0)) {
				om.KillRecursive(o);
				return nullptr;
			}
			return o;
		}

		int HandleData(Data_r&& dr) {
			// 读出 target
			uint32_t target;
			if (dr.ReadFixed(target)) return __LINE__;
			{
				int r = HandleTargetMessage(target, dr);
				if (r == 0) return 0;		// continue
				if (r < 0) return __LINE__;
			}

			// 读出序号
			int32_t serial;
			if (dr.Read(serial)) return __LINE__;

			// 如果是 Response 包，则在 req 字典查找。如果找到就 解包 + 传递 + 协程放行
			if (serial > 0) {
				if (auto iter = reqs.find(serial); iter != reqs.end()) {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					iter->second.second = std::move(o);
					iter->second.first.cancel();
				}
			} else {
				// 如果是 Push 包，且有提供 ReceivePush 处理函数，就 解包 + 传递
				if (serial == 0) {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					if (ReceiveTargetPush(target, std::move(o))) return __LINE__;
				}
				// 如果是 Request 包，且有提供 ReceiveRequest 处理函数，就 解包 + 传递
				else {
					auto o = ReadFrom(dr);
					if (!o) return __LINE__;
					if (ReceiveTargetRequest(target, -serial, std::move(o))) return __LINE__;
				}
				if (stoped) return __LINE__;
			}
			return 0;
		}
	};

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
		return peer && !peer->IsStoped();
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
		peer->serverEP = { addr, port };
		if (auto r = co_await peer->Connect(5s)) co_return __LINE__;								// 开始连接. 超时 5 秒
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
	bool Client::TryPop(Package<Shared<T>>& ro, std::optional<uint32_t> serverId) {
		if (!*this) return false;																	 // 异常：已断开
		std::deque<Package<ObjBase_s>>* os = nullptr;
		if (serverId.has_value()) {																	 // 如果有指定 serverId
			os = &peer->recvObjs[serverId.value()];													 // 定位到队列
			if (os->empty()) return false;															 // 空: 短路出去
		} else {
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

	inline void Client::SendTo(uint32_t const& target, int32_t const& serial, Span const& dr) {
		if (!*this) return;
		xx::Data d;
		d.Reserve(dr.len + 32);
		auto bak = d.WriteJump<false>(sizeof(uint32_t));
		d.WriteFixed<false>(target);
		d.WriteVarInteger<false>(serial);
		d.WriteBufSpans(dr);
		d.WriteFixedAt(bak, (uint32_t)(d.len - 4));
		peer->Send(std::move(d));
	}
	inline void Client::Send(Data&& d) {
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
