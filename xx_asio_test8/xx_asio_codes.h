#pragma once
#include <xx_obj.h>
#include <iostream>
#include <charconv>
#include <asio.hpp>
using namespace std::literals;
using namespace std::literals::chrono_literals;
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

namespace xx {

	asio::awaitable<void> Timeout(std::chrono::steady_clock::duration const& d) {
		asio::steady_timer t(co_await asio::this_coro::executor);
		t.expires_after(d);
		co_await t.async_wait(use_nothrow_awaitable);
	}

	template<typename T>
	concept PeerDeriveType = requires(T t) {
		t.shared_from_this();	// struct PeerDeriveType : std::enable_shared_from_this< PeerDeriveType >
		t.HandleMessage();		// 核心消息处理函数( 参数为 xx::Data& msg, 需要自行 msg.RemoveFront )
	};

	// 成员函数 / 组合探测器
	template<typename T> concept HasPeerStartAfter = requires(T t) { t.StartAfter(); };
	template<typename T> concept HasPeerStopAfter = requires(T t) { t.StopAfter(); };

	template<typename PeerDeriveType, bool hasRequestCode = false, bool hasTimeoutCode = false>
	struct PeerCode : asio::noncopyable {
		asio::io_context& ioc;
		asio::ip::tcp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<Data> writeQueue;
		bool stoped = true;

		PeerCode(asio::io_context& ioc_, asio::ip::tcp::socket&& socket_)
			: ioc(ioc_)
			, socket(std::move(socket_))
			, writeBlocker(ioc_)
		{
		}

		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			if constexpr (hasRequestCode) {
				((PeerDeriveType*)(this))->reqAutoId = 0;
			}
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Read(); }, asio::detached);
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Write(); }, asio::detached);
			if constexpr(HasPeerStartAfter<PeerDeriveType>) {
				((PeerDeriveType*)(this))->StartAfter();
			}
		}

		void Stop() {
			if (stoped) return;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			if constexpr (hasTimeoutCode) {
				((PeerDeriveType*)(this))->timeouter.cancel();
			}
			if constexpr (hasRequestCode) {
				for (auto& kv : ((PeerDeriveType*)(this))->reqs) {
					kv.second.first.cancel();
				}
			}
			if constexpr (HasPeerStopAfter<PeerDeriveType>) {
				((PeerDeriveType*)(this))->StopAfter();
			}
		}

		// for client dial connect to server only
		asio::awaitable<int> Connect(asio::ip::address const& ip, uint16_t const& port, std::chrono::steady_clock::duration const& d = 5s) {
			if (!stoped) co_return 1;
			auto r = co_await(socket.async_connect({ ip, port }, use_nothrow_awaitable) || Timeout(d));
			if (r.index()) co_return 2;
			if (auto& [e] = std::get<0>(r); e) co_return 3;
			Start();
			co_return 0;
		}

		void Send(Data&& msg) {
			if (stoped) return;
			writeQueue.emplace_back(std::move(msg));
			writeBlocker.cancel_one();
		}

	protected:
		asio::awaitable<void> Read() {
			for (Data msg(1024 * 256);;) {
				auto [ec, n] = co_await socket.async_read_some(asio::buffer(msg.buf + msg.len, msg.cap - msg.len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (!n) continue;
				if constexpr (hasTimeoutCode) {
					if (((PeerDeriveType*)(this))->stoping) {
						msg.Clear();
						continue;
					}
				}
				else {
					msg.len += n;
					((PeerDeriveType*)(this))->HandleMessage(msg);
				}
				if (stoped) co_return;
				msg.offset = 0;
			}
			Stop();
		}

		asio::awaitable<void> Write() {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					co_await writeBlocker.async_wait(use_nothrow_awaitable);
					if (stoped) co_return;
				}
				auto& msg = writeQueue.front();
				auto buf = msg.buf;
				auto len = msg.len;
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

	template<typename ServerDeriveType>
	struct ServerCode : asio::noncopyable {
		asio::io_context ioc;				// .run() execute
		asio::signal_set signals;
		ServerCode() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

		// 需要目标 Peer 实现( Server&, &&socket ) 构造函数
		template<typename Peer>
		void Listen(uint16_t const& port) {
			asio::co_spawn(ioc, [this, port]()->asio::awaitable<void> {
				asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
				for (;;) {
					asio::ip::tcp::socket socket(ioc);
					if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
						std::make_shared<Peer>(*(ServerDeriveType*)this, std::move(socket))->Start();
					}
				}
				}, asio::detached);
		}
	};

	// 为 peer 附加超时代码段落
	template<typename PeerDeriveType>
	struct PeerTimeoutCode {

		asio::steady_timer timeouter;
		bool stoping = false;

		PeerTimeoutCode(asio::io_context& ioc) : timeouter(ioc) {}

		void DelayStop(std::chrono::steady_clock::duration const& d) {
			if (stoping || ((PeerDeriveType*)(this))->stoped) return;
			stoping = true;
			ResetTimeout(d);
		}

		void ResetTimeout(std::chrono::steady_clock::duration const& d) {
			timeouter.expires_after(d);
			timeouter.async_wait([this](auto&& ec) {
				if (ec) return;
				((PeerDeriveType*)(this))->Stop();
			});
		}
	};

	// 为 peer 附加 Send( Obj )( SendPush, SendRequest, SendResponse ) 等 相关功能
	/* 需要手工添加下列代码到相应函数
	void HandleMessage(....msg) {
		...
		反序列化出 serial, o
		TriggerReq( serial, std::move(o) );
		 ...
	}
	*/
	template<typename PeerDeriveType>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;

		int TriggerReq(int32_t const& serial, ObjBase_s&& o) {
			if (auto iter = reqs.find(serial); iter != reqs.end()) {
				iter->second.second = std::move(o);
				iter->second.first.cancel();
				return 0;
			}
			return __LINE__;
		}

		void FillTo(ObjManager& om, Data& d, ObjBase_s const& o) {
			auto bak = d.WriteJump(sizeof(uint32_t));	// package len
			d.WriteVarInteger(reqAutoId);				// serial
			om.WriteTo(d, o);							// typeid + data
			d.WriteFixedAt(bak, d.len);					// fill package len
		}

		asio::awaitable<ObjBase_s> SendRequest(ObjManager& om, ObjBase_s const& o, std::chrono::steady_clock::duration timeoutSecs = 15s) {
			// 创建请求
			auto iter = reqs.emplace(++reqAutoId, std::make_pair(asio::steady_timer(((PeerDeriveType*)(this))->ioc, std::chrono::steady_clock::now() + timeoutSecs), ObjBase_s())).first;
			Data d;
			FillTo(d, o);
			((PeerDeriveType*)(this))->Send(std::move(d));
			co_await iter->second.first.async_wait(use_nothrow_awaitable);	// 等回包 或 超时
			auto r = std::move(iter->second.second);	// 拿出 网络回包
			reqs.erase(iter);	// 移除请求
			co_return r;	// 返回 网络回包( 超时 为 空 )
		}

		// todo: SendResponse, SendPush
	};

	// todo: more xxxxCode here
}
