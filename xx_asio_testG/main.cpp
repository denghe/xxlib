#include <xx_asio_ioc.h>
#include <random>
#include <ikcp.h>

// 写一个 kcp client, 接口等同 tcp, 最后应该要封到 lua 去用

namespace xx {

	// tips: need wrap by xx::Shared, co_spawn hold for ensure life cycle
	template<typename PeerDeriveType, typename IOCType, size_t writeQueueSizeLimit = 50000>
	struct PeerUdpBaseCode : asio::noncopyable {
		IOCType& ioc;
		asio::ip::udp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<std::pair<asio::ip::udp::endpoint, xx::DataShared>> writeQueue;
		bool stoped{ true };
		std::string lastUdpBaseErr;

		explicit PeerUdpBaseCode(IOCType& ioc_) : ioc(ioc_), socket(ioc_), writeBlocker(ioc_) {}
		PeerUdpBaseCode(IOCType& ioc_, asio::ip::udp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), writeBlocker(ioc_) {}

		// will be call after peer created
		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			co_spawn(ioc, PEERTHIS->Read(xx::SharedFromThis(this)), detached);	// call PeerDeriveType's member func: awaitable<void> Read(auto memHolder)
			co_spawn(ioc, Write(xx::SharedFromThis(this)), detached);
			if constexpr (Has_Start_<PeerDeriveType>) {
				PEERTHIS->Start_();
			}
		}

		bool IsStoped() const {
			return stoped;
		}

		bool Stop() {
			if (stoped) return false;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			if constexpr (Has_Stop_<PeerDeriveType>) {
				PEERTHIS->Stop_();
			}
			return true;
		}

		// tips: broadcast, DataShared is recommend
		template<typename D>
		void SendTo(asio::ip::udp::endpoint const& ep, D&& d) {
			static_assert(std::is_base_of_v<xx::Data_r, std::decay_t<D>> || std::is_same_v<std::decay_t<D>, xx::DataShared>);
			if (stoped) return;
			if (!d) return;
			writeQueue.emplace_back(ep, std::forward<D>(d));
			writeBlocker.cancel_one();
		}

	protected:

		// continues send data in writeQueue
		awaitable<void> Write(auto memHolder) {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					co_await writeBlocker.async_wait(use_nothrow_awaitable);
					if (stoped) co_return;
				}
				if constexpr (writeQueueSizeLimit) {
					if (writeQueue.size() > writeQueueSizeLimit) {
						lastUdpBaseErr = "writeQueue.size() > " + std::to_string(writeQueueSizeLimit);
						break;
					}
				}
				auto& msg = writeQueue.front();
				auto buf = msg.second.GetBuf();
				auto len = msg.second.GetLen();
			LabBegin:;
				auto [ec, n] = co_await socket.async_send_to(asio::buffer(buf, len), msg.first, use_nothrow_awaitable);
				if (ec) {
					lastUdpBaseErr = "asio::async_send_to error: " + std::string(ec.message());
					break;
				}
				if (stoped) co_return;
				if (n < len) {
					len -= n;
					buf += n;
					goto LabBegin;
				}
				writeQueue.pop_front();
			}
			Stop();
		}
	};
}

struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
};

struct KcpClientPeer : xx::PeerUdpBaseCode<KcpClientPeer, IOC> {
	using BT = xx::PeerUdpBaseCode<KcpClientPeer, IOC>;
	using BT::BT;
	asio::ip::udp::endpoint serverEP;
	xx::Data secret, recv;
	ikcpcb* kcp = nullptr;
	uint32_t conv = 0;
	int64_t createMS = 0;
	uint32_t nextUpdateMS = 0;
	bool busy = false;

	// client: set target ip + port
	template<typename IP>
	void SetServerIPPort(IP&& ip, asio::ip::port_type const& port) {
		assert(!kcp);
		assert(!busy);
		serverEP = xx::AddressToEndpoint<asio::ip::udp::endpoint>(std::forward<IP>(ip), port);
	}

protected:
	inline void FillSecret() {
		std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<int> dis1(5, 19);	// 4: old protocol
		std::uniform_int_distribution<int> dis2(0, 255);
		auto siz = dis1(rng);
		secret.Resize(siz);
		for (size_t i = 0; i < siz; i++) {
			secret[i] = dis2(rng);
		}
	}

	inline void InitKcp() {
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

			auto self = (KcpClientPeer*)user;
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
	inline awaitable<int> Connect(std::chrono::steady_clock::duration d = 5s) {
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
	
	inline void Stop_() {
		assert(kcp);
		assert(!busy);
		ikcp_release(kcp);
		kcp = nullptr;
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

	inline awaitable<void> Read(auto memHolder) {
		auto buf = std::make_unique<uint8_t[]>(1024 * 1024);
		for (;;) {
			asio::ip::udp::endpoint ep;	// unused here
			auto [ec, len] = co_await socket.async_receive_from(asio::buffer(buf.get(), 1024 * 1024), ep, use_nothrow_awaitable);
			if (ec) break;
			if (stoped) co_return;
			if (!len) continue;

			// decrypt
			size_t j = 0;
			for (size_t i = 4; i < len; i++) {		// 4: skip convId
				buf[i] = buf[i] ^ secret[j++];
				if (j == secret.len) {
					j = 0;
				}
			}

			// put raw data into kcp
			if (int r = ikcp_input(kcp, (char*)buf.get(), (long)len)) {
				std::cout << "ikcp_input error. r = " << r << std::endl;
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

				// 调用用户数据处理函数
				//Receive();	// todo
				xx::Cout("\n kcp decoded recv = ", recv);

				if (IsStoped()) co_return;
			} while (true);
		}
	LabStop:
		Stop();
	}

	inline awaitable<void> Update(auto memHolder) {
		while(kcp) {
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
};

int main() {
	IOC ioc;
	co_spawn(ioc, [&]()->awaitable<void> {
		auto kp = xx::Make<KcpClientPeer>(ioc);
		kp->SetServerIPPort("127.0.0.1", 20000);
		auto r = co_await kp->Connect();
		std::cout << " r = " << r << " ";
		while (true) {
			co_await xx::Timeout(10ms);
			std::cout << ".";
			std::cout.flush();
		}
	}, detached);
	return ioc.run();
}
