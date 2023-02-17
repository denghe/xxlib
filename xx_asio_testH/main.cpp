#include <xx_asio_ioc.h>
#include <ikcp.h>


int main() {



	return 0;
}






//// xx_asio_kcp_base
//namespace xx {
//
//	// tips: need wrap by xx::Shared, co_spawn hold for ensure life cycle
//	template<typename PeerDeriveType, typename IOCType, size_t writeQueueSizeLimit = 50000>
//	struct PeerKcpBaseCode : asio::noncopyable {
//		IOCType& ioc;
//		asio::ip::udp::socket socket;
//		asio::steady_timer kcpUpdater;
//		std::deque<xx::DataShared> writeQueue;
//		bool stoped = true;
//		std::string lastKcpBaseErr;
//
//		explicit PeerKcpBaseCode(IOCType& ioc_) : ioc(ioc_), socket(ioc_), kcpUpdater(ioc_) {}
//		PeerKcpBaseCode(IOCType& ioc_, asio::ip::udp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), kcpUpdater(ioc_) {}
//
//		// will be call after peer created
//		void Start() {
//			if (!stoped) return;
//			stoped = false;
//			kcpUpdater.expires_at(std::chrono::steady_clock::time_point::max());
//			writeQueue.clear();
//			co_spawn(ioc, [this, self = xx::SharedFromThis(this)] { return PEERTHIS->Read(); }, detached);	// call PeerDeriveType's member func: awaitable<void> Read()
//			co_spawn(ioc, [self = xx::SharedFromThis(this)] { return self->Write(); }, detached);
//			if constexpr (Has_Start_<PeerDeriveType>) {
//				PEERTHIS->Start_();
//			}
//		}
//
//		bool IsStoped() const {
//			return stoped;
//		}
//
//		bool Stop() {
//			if (stoped) return false;
//			stoped = true;
//			socket.close();
//			kcpUpdater.cancel();
//			if constexpr (Has_Stop_<PeerDeriveType>) {
//				PEERTHIS->Stop_();
//			}
//			return true;
//		}
//
//		// tips: broadcast, DataShared is recommend
//		template<typename D>
//		void Send(D&& d) {
//			static_assert(std::is_base_of_v<xx::Data_r, std::decay_t<D>> || std::is_same_v<std::decay_t<D>, xx::DataShared>);
//			if (stoped) return;
//			if (!d) return;
//			writeQueue.emplace_back(std::forward<D>(d));
//			kcpUpdater.cancel_one();
//		}
//
//		// for client dial connect to server only
//		awaitable<int> Connect(asio::ip::address ip, uint16_t port, std::chrono::steady_clock::duration d = 5s) {
//			if (!stoped) co_return 1;
//			socket = asio::ip::udp::socket(ioc);    // for macos
//			auto r = co_await(socket.async_connect({ ip, port }, use_nothrow_awaitable) || Timeout(d));
//			if (r.index()) co_return 2;
//			if (auto& [e] = std::get<0>(r); e) co_return 3;
//			Start();
//			co_return 0;
//		}
//
//	protected:
//
//		// continues send data in writeQueue
//		awaitable<void> Write() {
//			while (socket.is_open()) {
//				if (writeQueue.empty()) {
//					co_await kcpUpdater.async_wait(use_nothrow_awaitable);
//					if (stoped) co_return;
//				}
//				if constexpr (writeQueueSizeLimit) {
//					if (writeQueue.size() > writeQueueSizeLimit) {
//						lastTcpBaseErr = "writeQueue.size() > " + std::to_string(writeQueueSizeLimit);
//						break;
//					}
//				}
//				auto& msg = writeQueue.front();
//				auto buf = msg.GetBuf();
//				auto len = msg.GetLen();
//			LabBegin:
//				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
//				if (ec) {
//					lastTcpBaseErr = "asio::async_write error: " + std::string(ec.message());
//					break;
//				}
//				if (stoped) co_return;
//				if (n < len) {
//					len -= n;
//					buf += n;
//					goto LabBegin;
//				}
//				writeQueue.pop_front();
//			}
//			Stop();
//		}
//	};
//}
//
//// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
//struct IOC : xx::IOCBase {
//	using IOCBase::IOCBase;
//};


//int RunServer() {
//	IOC ioc(1);
//
//	ioc.run();
//	return 0;
//}
//
//int RunClient() {
//	IOC ioc(1);
//
//	ioc.run();
//	return 0;
//}
//
//int main() {
//	//std::array<std::thread, 1> ts;
//	//for (auto& t : ts) {
//	//	t = std::thread([] { Run(); });
//	//	t.detach();
//	//}
//	//std::cin.get();
//	return 0;
//}

