#pragma once
#include <xx_asio_ioc.h>

namespace xx {

	// 各种成员函数是否存在的检测模板
	template<class T, class = void> struct _Has_Start_ : std::false_type {};
	template<class T> struct _Has_Start_<T, std::void_t<decltype(std::declval<T&>().Start_())>> : std::true_type {};
	template<class T> constexpr bool Has_Start_ = _Has_Start_<T>::value;

	template<class T, class = void> struct _Has_Stop_ : std::false_type {};
	template<class T> struct _Has_Stop_<T, std::void_t<decltype(std::declval<T&>().Stop_())>> : std::true_type {};
	template<class T> constexpr bool Has_Stop_ = _Has_Stop_<T>::value;

	// 最终 Peer 应使用 xx::Shared 包裹使用，以方便 co_spawn 捕获加持, 确保生命周期长于协程
	template<typename PeerDeriveType, typename IOCType, size_t writeQueueSizeLimit = 100>
	struct PeerTcpBaseCode : asio::noncopyable {
		IOCType& ioc;
		asio::ip::tcp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<xx::DataShared> writeQueue;
		bool stoped = true;

		explicit PeerTcpBaseCode(IOCType& ioc_) : ioc(ioc_), socket(ioc_), writeBlocker(ioc_) {}
		PeerTcpBaseCode(IOCType& ioc_, asio::ip::tcp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), writeBlocker(ioc_) {}

		// 初始化。通常于 peer 创建后立即调用
		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			co_spawn(ioc, [this, self = xx::SharedFromThis(this)] { return PEERTHIS->Read(); }, detached);	// 派生类需要提供 awaitable<void> Read() 的实现
			co_spawn(ioc, [self = xx::SharedFromThis(this)] { return self->Write(); }, detached);
			if constexpr (Has_Start_<PeerDeriveType>) {
				PEERTHIS->Start_();
			}
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
			if constexpr (Has_Stop_<PeerDeriveType>) {
				PEERTHIS->Stop_();
			}
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

		// for client dial connect to server only
		awaitable<int> Connect(asio::ip::address ip, uint16_t port, std::chrono::steady_clock::duration d = 5s) {
			if (!stoped) co_return 1;
			socket = asio::ip::tcp::socket(ioc);    // for macos
			auto r = co_await(socket.async_connect({ ip, port }, use_nothrow_awaitable) || Timeout(d));
			if (r.index()) co_return 2;
			if (auto& [e] = std::get<0>(r); e) co_return 3;
			Start();
			co_return 0;
		}

	protected:

		// 不停的将 writeQueue 发出的 写 协程函数主体
		awaitable<void> Write() {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					co_await writeBlocker.async_wait(use_nothrow_awaitable);
					if (stoped) co_return;
				}
				if constexpr (writeQueueSizeLimit) {
					if (writeQueue.size() > writeQueueSizeLimit)
						break;
				}
				auto& msg = writeQueue.front();
				auto buf = msg.GetBuf();
				auto len = msg.GetLen();
			LabBegin:
				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
				if (ec)
					break;
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
