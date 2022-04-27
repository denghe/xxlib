﻿#pragma once

// 使用方法：xcode 需要配置 c++ 为 20 版本 并且去 other c++ flags 看看 如果有 17 字样 删掉
// android 某 gradle 某行加点料 cppFlags "-frtti -fexceptions -fsigned-char -std=c++20 -fcoroutines-ts"

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
using asio::co_spawn;
using asio::awaitable;
using asio::detached;

namespace xx {

	awaitable<void> Timeout(std::chrono::steady_clock::duration const& d) {
		asio::steady_timer t(co_await asio::this_coro::executor);
		t.expires_after(d);
		co_await t.async_wait(use_nothrow_awaitable);
	}

	template<typename ServerDeriveType>
	struct IOCCode : asio::noncopyable {
		asio::io_context ioc;				// .run() execute
		asio::signal_set signals;
		IOCCode() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

		// 需要目标 Peer 实现( Server&, &&socket ) 构造函数
		// 如果需要灵活控制 socket 根据一些状态开关，可以复制小改这个函数, 自己实现
		template<typename Peer>
		void Listen(uint16_t const& port) {
			co_spawn(ioc, [this, port]()->awaitable<void> {
				asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
				for (;;) {
					asio::ip::tcp::socket socket(ioc);
					if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
						std::make_shared<Peer>(*(ServerDeriveType*)this, std::move(socket))->Start();
					}
				}
			}, detached);
		}
	};

	// 代码片段 / 成员函数 探测器系列
#ifndef __ANDROID__
	// struct PeerDeriveType : std::enable_shared_from_this< PeerDeriveType >
	template<typename T> concept PeerDeriveType = requires(T t) { t.shared_from_this(); };

	// 用于提供消息处理功能. 如果有继承 PeerRequestCode 则无需提供。具体写法可参考之
	template<typename T> concept Has_Peer_HandleMessage = requires(T t) { t.HandleMessage((uint8_t*)0, 0); };

	// 用于补充 Start 函数功能
	template<typename T> concept Has_Peer_Start_ = requires(T t) { t.Start_(); };

	// 用于补充 Stop 函数功能
	template<typename T> concept Has_Peer_Stop_ = requires(T t) { t.Stop_(); };

	// 用于检测是否继承了 PeerTimeoutCode 代码片段
	template<typename T> concept Has_PeerTimeoutCode = requires(T t) { t.ResetTimeout(1s); };

	// 用于检测是否继承了 PeerRequestCode 代码片段
	template<typename T> concept Has_PeerRequestCode = requires(T t) { t.ReadFrom(std::declval<Data_r&>()); };

	// PeerRequestCode 片段依赖的函数
	template<typename T> concept Has_Peer_ReceivePush = requires(T t) { t.ReceivePush(ObjBase_s()); };
	template<typename T> concept Has_Peer_ReceiveRequest = requires(T t) { t.ReceiveRequest(0, ObjBase_s()); };
	template<typename T> concept Has_Peer_ReceiveTargetPush = requires(T t) { t.ReceiveTargetPush(0, ObjBase_s()); };
	template<typename T> concept Has_Peer_ReceiveTargetRequest = requires(T t) { t.ReceiveTargetRequest(0, 0, ObjBase_s()); };
	template<typename T> concept Has_Peer_HandleTargetMessage = requires(T t) { t.HandleTargetMessage(0, std::declval<Data_r&>()); };
#else
	template<class T, class = void>
	struct _Has_Peer_HandleMessage : std::false_type {};
	template<class T>
	struct _Has_Peer_HandleMessage<T, std::void_t<decltype(std::declval<T&>().HandleMessage((uint8_t*)0, 0))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_HandleMessage = _Has_Peer_HandleMessage<T>::value;

	template<class T, class = void>
	struct _Has_Peer_Start_ : std::false_type {};
	template<class T>
	struct _Has_Peer_Start_<T, std::void_t<decltype(std::declval<T&>().Start_())>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_Start_ = _Has_Peer_Start_<T>::value;

	template<class T, class = void>
	struct _Has_Peer_Stop_ : std::false_type {};
	template<class T>
	struct _Has_Peer_Stop_<T, std::void_t<decltype(std::declval<T&>().Stop_())>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_Stop_ = _Has_Peer_Stop_<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ResetTimeout : std::false_type {};
	template<class T>
	struct _Has_Peer_ResetTimeout<T, std::void_t<decltype(std::declval<T&>().ResetTimeout(1s))>> : std::true_type {};
	template<class T>
	constexpr bool Has_PeerTimeoutCode = _Has_Peer_ResetTimeout<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ReadFrom : std::false_type {};
	template<class T>
	struct _Has_Peer_ReadFrom<T, std::void_t<decltype(std::declval<T&>().ReadFrom(std::declval<Data_r&>()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_PeerRequestCode = _Has_Peer_ReadFrom<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ReceivePush : std::false_type {};
	template<class T>
	struct _Has_Peer_ReceivePush<T, std::void_t<decltype(std::declval<T&>().ReceivePush(ObjBase_s()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_ReceivePush = _Has_Peer_ReceivePush<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ReceiveRequest : std::false_type {};
	template<class T>
	struct _Has_Peer_ReceiveRequest<T, std::void_t<decltype(std::declval<T&>().ReceiveRequest(0, ObjBase_s()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_ReceiveRequest = _Has_Peer_ReceiveRequest<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ReceiveTargetPush : std::false_type {};
	template<class T>
	struct _Has_Peer_ReceiveTargetPush<T, std::void_t<decltype(std::declval<T&>().ReceiveTargetPush(0, ObjBase_s()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_ReceiveTargetPush = _Has_Peer_ReceiveTargetPush<T>::value;

	template<class T, class = void>
	struct _Has_Peer_ReceiveTargetRequest : std::false_type {};
	template<class T>
	struct _Has_Peer_ReceiveTargetRequest<T, std::void_t<decltype(std::declval<T&>().ReceiveTargetRequest(0, 0, ObjBase_s()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_ReceiveTargetRequest = _Has_Peer_ReceiveTargetRequest<T>::value;

	template<class T, class = void>
	struct _Has_Peer_HandleTargetMessage : std::false_type {};
	template<class T>
	struct _Has_Peer_HandleTargetMessage<T, std::void_t<decltype(std::declval<T&>().HandleTargetMessage(0, std::declval<Data_r&>()))>> : std::true_type {};
	template<class T>
	constexpr bool Has_Peer_HandleTargetMessage = _Has_Peer_HandleTargetMessage<T>::value;
#endif




#define PEERTHIS ((PeerDeriveType*)(this))

	template<typename PeerDeriveType, size_t readBufSize = 524288>
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
			if constexpr (Has_PeerRequestCode<PeerDeriveType>) {
				PEERTHIS->reqAutoId = 0;
			}
			co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Read(); }, detached);
			co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Write(); }, detached);
			if constexpr(Has_Peer_Start_<PeerDeriveType>) {
				PEERTHIS->Start_();
			}
		}

		void Stop() {
			if (stoped) return;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			if constexpr (Has_PeerTimeoutCode<PeerDeriveType>) {
				PEERTHIS->timeouter.cancel();
			}
			if constexpr (Has_PeerRequestCode<PeerDeriveType>) {
				for (auto& kv : PEERTHIS->reqs) {
					kv.second.first.cancel();
				}
			}
			if constexpr (Has_Peer_Stop_<PeerDeriveType>) {
				PEERTHIS->Stop_();
			}
		}

		// for client dial connect to server only
		awaitable<int> Connect(asio::ip::address const& ip, uint16_t const& port, std::chrono::steady_clock::duration const& d = 5s) {
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

		bool Alive() const {
			if constexpr (Has_PeerTimeoutCode<PeerDeriveType>) {
				return !stoped && !PEERTHIS->stoping;
			}
			else return !stoped;
		}

	protected:
		awaitable<void> Read() {
			uint8_t buf[readBufSize];
			size_t len = 0;
			for (;;) {
				auto [ec, n] = co_await socket.async_read_some(asio::buffer(buf + len, sizeof(buf) - len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (!n) continue;
				if constexpr (Has_PeerTimeoutCode<PeerDeriveType>) {
					if (PEERTHIS->stoping) {
						len = 0;
						continue;
					}
				}
				len += n;
				if constexpr (Has_Peer_HandleMessage<PeerDeriveType>) {
					n = PEERTHIS->HandleMessage(buf, len);
					if (stoped) co_return;
					if constexpr (Has_PeerTimeoutCode<PeerDeriveType>) {
						if (PEERTHIS->stoping) co_return;
					}
					if (!n) break;
				}
				len -= n;
				memmove(buf, buf + n, len);
			}
			Stop();
		}

		awaitable<void> Write() {
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


	// 为 peer 附加 超时 & 延迟 Stop 代码段落
	template<typename PeerDeriveType>
	struct PeerTimeoutCode {

		asio::steady_timer timeouter;
		bool stoping = false;

		PeerTimeoutCode(asio::io_context& ioc) : timeouter(ioc) {}

		void DelayStop(std::chrono::steady_clock::duration const& d) {
			if (stoping || PEERTHIS->stoped) return;
			stoping = true;
			ResetTimeout(d);
		}

		void ResetTimeout(std::chrono::steady_clock::duration const& d) {
			timeouter.expires_after(d);
			timeouter.async_wait([this](auto&& ec) {
				if (ec) return;
				PEERTHIS->Stop();
			});
		}

		void foo() {}
	};


	// 为 peer 附加 Send( Obj )( SendPush, SendRequest, SendResponse ) 等 相关功能
	/* 
	需要手工添加一些处理函数
	如果 containTarget == true 那么

		// 收到 目标的 请求( 返回非 0 表示失败，会 Stop )
		int ReceiveTargetRequest(uint32_t target, xx::ObjBase_s&& o) {

		// 收到 目标的 推送( 返回非 0 表示失败，会 Stop )
		int ReceiveTargetPush(uint32_t target, xx::ObjBase_s&& o) {
			// todo: handle o
			// 非法 o 随手处理下 om.KillRecursive(o);
			return 0;	// 要掐线就返回非 0
		}

		// 拦截处理特殊 target 路由需求( 返回 0 表示已成功处理，   返回 正数 表示不拦截,    返回负数表示 出错 )
		int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
			if (target == 0xFFFFFFFF) {
				// ...
				return 0;
			}
			return 1;	// passthrough
		}

	否则

		// 收到 请求( 返回非 0 表示失败，会 Stop )
		int ReceiveRequest(xx::ObjBase_s&& o) {

		// 收到 推送( 返回非 0 表示失败，会 Stop )
		int ReceivePush(xx::ObjBase_s&& o) {
			// todo: handle o
			// 非法 o 随手处理下 om.KillRecursive(o);
			return 0;	// 要掐线就返回非 0
		}
	*/
	template<typename PeerDeriveType, bool containTarget = false, size_t sendCap = 8192, size_t maxDataLen = 524288>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;
		ObjManager& om;

		PeerRequestCode(ObjManager& om_) : om(om_) {}

		template<typename PKG = ObjBase, typename ... Args>
		void SendCore(uint32_t const& target, int32_t const& serial, Args const &... args) {
			Data d;
			d.Reserve(sendCap);
			auto bak = d.WriteJump<false>(sizeof(uint32_t));
			if constexpr(containTarget) {
				d.WriteFixed<false>(target);
			}
			d.WriteVarInteger<false>(serial);
			// 传统写包
			if constexpr (std::is_same_v<xx::ObjBase, PKG>) {
				om.WriteTo(d, args...);
			}
			// 直写 cache buf 包
			else if constexpr (std::is_same_v<xx::Span, PKG>) {
				d.WriteBufSpans(args...);
			}
			// 使用 目标类的静态函数 快速填充 buf
			else {
				PKG::WriteTo(d, args...);
			}
			d.WriteFixedAt(bak, (uint32_t)(d.len - 4));
			PEERTHIS->Send(std::move(d));
		}

		template<typename PKG = xx::ObjBase, typename ... Args, class = std::enable_if_t<containTarget>>
		void SendResponse(uint32_t const& target, int32_t const& serial, Args const& ... args) {
			this->template SendCore<PKG>(target, serial, args...);
		}

		template<typename PKG = xx::ObjBase, typename ... Args, class = std::enable_if_t<containTarget>>
		void SendPush(uint32_t const& target, Args const& ... args) {
			this->template SendCore<PKG>(target, 0, args...);
		}

		template<typename PKG = xx::ObjBase, typename ... Args, class = std::enable_if_t<containTarget>>
		awaitable<ObjBase_s> SendRequest(uint32_t const& target, std::chrono::steady_clock::duration timeoutSecs, Args const& ... args) {
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto iter = reqs.emplace(reqAutoId, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + timeoutSecs), ObjBase_s())).first;
			this->template SendCore<PKG>(target, -reqAutoId, args...);
			co_await iter->second.first.async_wait(use_nothrow_awaitable);
			auto r = std::move(iter->second.second);
			reqs.erase(iter);
			co_return r;
		}

		template<typename PKG = ObjBase, typename ... Args, class = std::enable_if_t<!containTarget>>
		void SendResponse(int32_t const& serial, Args const &... args) {
			this->template SendCore<PKG>(0, serial, args...);
		}

		template<typename PKG = xx::ObjBase, typename ... Args, class = std::enable_if_t<!containTarget>>
		void SendPush(Args const& ... args) {
			this->template SendCore<PKG>(0, 0, args...);
		}

		template<typename PKG = xx::ObjBase, typename ... Args, class = std::enable_if_t<!containTarget>>
		awaitable<ObjBase_s> SendRequest(std::chrono::steady_clock::duration timeoutSecs, Args const& ... args) {
			reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
			auto iter = reqs.emplace(reqAutoId, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + timeoutSecs), ObjBase_s())).first;
			this->template SendCore<PKG>(0, -reqAutoId, args...);
			co_await iter->second.first.async_wait(use_nothrow_awaitable);
			auto r = std::move(iter->second.second);
			reqs.erase(iter);
			co_return r;
		}

		xx::ObjBase_s ReadFrom(xx::Data_r& dr) {
			xx::ObjBase_s o;
			if (om.ReadFrom(dr, o) || (o && o.typeId() == 0)) {
				om.KillRecursive(o);
				return {};
			}
			return o;
		}

		size_t HandleMessage(uint8_t* inBuf, size_t len) {
			if constexpr (Has_PeerTimeoutCode<PeerDeriveType>) {
				// 正在停止，直接吞掉所有数据
				if (PEERTHIS->stoping) return len;
			}

			// 取出指针备用
			auto buf = (uint8_t*)inBuf;
			auto end = (uint8_t*)inBuf + len;

			// 包头
			uint32_t dataLen = 0;
			uint32_t target = 0;	// containTarget == true 才用得到

			// 确保包头长度充足
			while (buf + sizeof(dataLen) <= end) {
				// 取长度
				dataLen = *(uint32_t*)buf;

				// 长度保护
				if (dataLen > maxDataLen) return 0;	// Stop

				// 计算包总长( 包头长 + 数据长 )
				auto totalLen = sizeof(dataLen) + dataLen;

				// 如果包不完整 就 跳出
				if (buf + totalLen > end) break;	// continue

				do {
					auto dr = xx::Data_r(buf + sizeof(dataLen), dataLen);

					// 读出 target
					if constexpr (containTarget) {
						if (dr.ReadFixed(target)) return 0;	// Stop
						if constexpr (Has_Peer_HandleTargetMessage<PeerDeriveType>) {
							int r = PEERTHIS->HandleTargetMessage(target, dr);
							if (r == 0) break;		// continue
							if (r < 0) return 0;	// Stop
							// passthrough
						}
					}

					// 读出序号
					int32_t serial;
					if (dr.Read(serial)) return 0;	// Stop

					// 如果是 Response 包，则在 req 字典查找。如果找到就 解包 + 传递 + 协程放行
					if (serial > 0) {
						if (auto iter = reqs.find(serial); iter != reqs.end()) {
							auto o = ReadFrom(dr);
							if (!o) return 0;	// Stop
							iter->second.second = std::move(o);
							iter->second.first.cancel();
						}
					}
					else {
						// 如果是 Push 包，且有提供 ReceivePush 处理函数，就 解包 + 传递
						if (serial == 0) {
							if constexpr (containTarget) {
								static_assert(!Has_Peer_ReceivePush<PeerDeriveType>);
								if constexpr (Has_Peer_ReceiveTargetPush<PeerDeriveType>) {
									auto o = ReadFrom(dr);
									if (!o) return 0;
									if (PEERTHIS->ReceiveTargetPush(target, std::move(o))) return 0;	// Stop
								}
							}
							else {
								static_assert(!Has_Peer_ReceiveTargetPush<PeerDeriveType>);
								if constexpr (Has_Peer_ReceivePush<PeerDeriveType>) {
									auto o = ReadFrom(dr);
									if (!o) return 0;		// Stop
									if (PEERTHIS->ReceivePush(std::move(o))) return 0;	// Stop
								}
							}
						}
						// 如果是 Request 包，且有提供 ReceiveRequest 处理函数，就 解包 + 传递
						else {
							if constexpr (containTarget) {
								static_assert(!Has_Peer_ReceiveRequest<PeerDeriveType>);
								if constexpr (Has_Peer_ReceiveTargetRequest<PeerDeriveType>) {
									auto o = ReadFrom(dr);
									if (!o) return 0;		// Stop
									if (PEERTHIS->ReceiveTargetRequest(target, -serial, std::move(o))) return 0;	// Stop
								}
							}
							else {
								static_assert(!Has_Peer_ReceiveTargetRequest<PeerDeriveType>);
								if constexpr (Has_Peer_ReceiveRequest<PeerDeriveType>) {
									auto o = ReadFrom(dr);
									if (!o) return 0;	// Stop
									if (PEERTHIS->ReceiveRequest(-serial, std::move(o))) return 0;	// Stop
								}
							}
						}
						if constexpr (Has_Peer_ReceivePush<PeerDeriveType> || Has_Peer_ReceiveRequest<PeerDeriveType>) {
							if (PEERTHIS->stoping || PEERTHIS->stoped) return 0;	// Stop
						}
					}
				} while (false);

				// 跳到下一个包的开头
				buf += totalLen;
			}

			// 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
			return buf - inBuf;
		}
	};

	// todo: more xxxxCode here

#undef PEERTHIS

}