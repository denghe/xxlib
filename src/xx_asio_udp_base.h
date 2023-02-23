#pragma once
#include <xx_asio_ioc.h>

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
