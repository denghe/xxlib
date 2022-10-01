#pragma once
#include <xx_asio_ioc.h>

namespace xx {
	// 最终 Peer 应使用 xx::Shared 包裹使用，以方便 co_spawn 捕获加持, 确保生命周期长于协程
	template<typename PeerDeriveType, typename ServerType>
	struct PeerTcpBaseCode : asio::noncopyable {
		ServerType& server;
		asio::ip::tcp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<xx::DataShared> writeQueue;
		bool stoped = true;

		PeerTcpBaseCode(ServerType& server_) : server(server_), socket(server_), writeBlocker(server_) {}
		PeerTcpBaseCode(ServerType& server_, asio::ip::tcp::socket&& socket_) : server(server_), socket(std::move(socket_)), writeBlocker(server_) {}

		// 初始化。通常于 peer 创建后立即调用
		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			co_spawn(server, [this, self = xx::SharedFromThis(this)] { return PEERTHIS->Read(); }, detached);	// 派生类需要提供 awaitable<void> Read() 的实现
			co_spawn(server, [self = xx::SharedFromThis(this)] { return self->Write(); }, detached);
		}

		// 判断是否已断开
		bool IsStoped() const {
			return stoped;
		}

		// 断开
		bool Stop() {
			if (stoped) return false;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			return true;
		}

		// 数据发送。参数只接受 xx::Data 或 xx::DataShared 这两种类型. 广播需求尽量发 DataShared
		template<typename D>
		void Send(D&& d) {
			static_assert(std::is_base_of_v<xx::Data_r, std::decay_t<D>> || std::is_same_v<std::decay_t<D>, xx::DataShared>);
			if (stoped) return;
			if (!d) return;
			writeQueue.emplace_back(std::forward<D>(d));
			writeBlocker.cancel_one();
		}

	protected:

		// 不停的将 writeQueue 发出的 写 协程函数主体
		awaitable<void> Write() {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					co_await writeBlocker.async_wait(use_nothrow_awaitable);
					if (stoped) co_return;
				}
				auto& msg = writeQueue.front();
				auto buf = msg.GetBuf();
				auto len = msg.GetLen();
			LabBegin:
				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
				if (ec) break;
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
