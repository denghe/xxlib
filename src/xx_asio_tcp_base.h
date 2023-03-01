#pragma once
#include <xx_asio_ioc.h>

namespace xx {

	// tips: need wrap by xx::Shared, co_spawn hold for ensure life cycle
	template<typename PeerDeriveType, typename IOCType, size_t writeQueueSizeLimit = 50000>
	struct PeerTcpBaseCode : asio::noncopyable {
		IOCType& ioc;
		asio::ip::tcp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<xx::DataShared> writeQueue;
		bool stoped = true;
		std::string lastTcpBaseErr;

		explicit PeerTcpBaseCode(IOCType& ioc_) : ioc(ioc_), socket(ioc_), writeBlocker(ioc_) {}
		PeerTcpBaseCode(IOCType& ioc_, asio::ip::tcp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), writeBlocker(ioc_) {}

		// will be call after peer created
		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			co_spawn(ioc, PEERTHIS->Read(xx::SharedFromThis(PEERTHIS)), detached);	// call PeerDeriveType's member func: awaitable<void> Read()
			co_spawn(ioc, PEERTHIS->Write(xx::SharedFromThis(PEERTHIS)) , detached);
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
		void Send(D&& d) {
			static_assert(std::is_base_of_v<xx::Data_r, std::decay_t<D>> || std::is_same_v<std::decay_t<D>, xx::DataShared>);
			if (stoped) return;
			if (!d) return;
			writeQueue.emplace_back(std::forward<D>(d));
			writeBlocker.cancel_one();
		}

		// for client dial connect to server only
		awaitable<int> Connect(asio::ip::tcp::endpoint ep, std::chrono::steady_clock::duration d = 5s) {
			if (!stoped) co_return 1;
			socket = asio::ip::tcp::socket(ioc);    // for macos
			auto r = co_await(socket.async_connect(ep, use_nothrow_awaitable) || Timeout(d));
			if (r.index()) co_return 2;
			if (auto& [e] = std::get<0>(r); e) co_return 3;
			Start();
			co_return 0;
		}
		awaitable<int> Connect(asio::ip::address ip, uint16_t port, std::chrono::steady_clock::duration d = 5s) {
			co_return co_await Connect({ ip, port }, d);
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
						lastTcpBaseErr = "writeQueue.size() > " + std::to_string(writeQueueSizeLimit);
						break;
					}
				}
				auto& msg = writeQueue.front();
				auto buf = msg.GetBuf();
				auto len = msg.GetLen();
			LabBegin:
				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
				if (ec) {
					lastTcpBaseErr = "asio::async_write error: " + std::string(ec.message());
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
