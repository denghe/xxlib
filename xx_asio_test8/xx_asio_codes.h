﻿#pragma once
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


	// 成员函数 / 组合探测器
	template<typename T>
	concept PeerDeriveType = requires(T t) {
		t.shared_from_this();	// struct PeerDeriveType : std::enable_shared_from_this< PeerDeriveType >
		t.HandleMessage((uint8_t*)0, 0);	// int ( char*, len ) 返回值为 已处理长度, 将从 buf 头部移除. 返回 <= 0 表示出错，将 Stop(). 
	};
	template<typename T> concept HasStart_ = requires(T t) { t.Start_(); };
	template<typename T> concept HasStop_ = requires(T t) { t.Stop_(); };

#define PEERTHIS ((PeerDeriveType*)(this))

	template<typename PeerDeriveType, bool hasTimeoutCode = true, bool hasRequestCode = true>
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
				PEERTHIS->reqAutoId = 0;
			}
			asio::co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Read(); }, asio::detached);
			asio::co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Write(); }, asio::detached);
			if constexpr(HasStart_<PeerDeriveType>) {
				PEERTHIS->Start_();
			}
		}

		void Stop() {
			if (stoped) return;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			if constexpr (hasTimeoutCode) {
				PEERTHIS->timeouter.cancel();
			}
			if constexpr (hasRequestCode) {
				for (auto& kv : PEERTHIS->reqs) {
					kv.second.first.cancel();
				}
			}
			if constexpr (HasStop_<PeerDeriveType>) {
				PEERTHIS->Stop_();
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

		bool Alive() const {
			if constexpr (hasTimeoutCode) {
				return !stoped && !PEERTHIS->stoping;
			}
			else return !stoped;
		}

	protected:
		asio::awaitable<void> Read() {
			uint8_t buf[1024 * 256];
			size_t len = 0;
			for (;;) {
				auto [ec, n] = co_await socket.async_read_some(asio::buffer(buf + len, sizeof(buf) - len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (!n) continue;
				if constexpr (hasTimeoutCode) {
					if (PEERTHIS->stoping) {
						len = 0;
						continue;
					}
				}
				len += n;
				n = PEERTHIS->HandleMessage(buf, len);
				if (stoped) co_return;
				if constexpr (hasTimeoutCode) {
					if (PEERTHIS->stoping) co_return;
				}
				if (!n) break;
				len -= n;
				memmove(buf, buf + n, len);
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
	};

	template<typename T> concept HasReceivePush = requires(T t) { t.ReceivePush(ObjBase_s()); };
	template<typename T> concept HasReceiveRequest = requires(T t) { t.ReceiveRequest(0, ObjBase_s()); };

	// 为 peer 附加 Send( Obj )( SendPush, SendRequest, SendResponse ) 等 相关功能
	/* 需要手工添加下列函数
	int ReceiveRequest(xx::ObjBase_s&& o) {
	int ReceivePush(xx::ObjBase_s&& o) {
		// todo: handle o
		// 非法 o 随手处理下 om.KillRecursive(o);
		return 0;	// 要掐线就返回非 0
	}
	*/
	template<typename PeerDeriveType, bool hasTimeoutCode = true>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;
		ObjManager& om;

		PeerRequestCode(ObjManager& om_) : om(om_) {}

		void FillTo(Data& d, int32_t const& serial, ObjBase_s const& o) {
			auto bak = d.WriteJump(sizeof(uint32_t));	// package len
			// todo: 网关包这里有个 投递地址 占用
			d.WriteVarInteger(serial);
			om.WriteTo(d, o);	// typeid + data
			d.WriteFixedAt(bak, (uint32_t)(d.len - sizeof(uint32_t)));	// fill package len
		}

		asio::awaitable<ObjBase_s> SendRequest(ObjBase_s const& o, std::chrono::steady_clock::duration timeoutSecs = 15s) {
			// 创建请求
			reqAutoId = (reqAutoId + 1) % 0x7fffffff;
			auto iter = reqs.emplace(reqAutoId, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + timeoutSecs), ObjBase_s())).first;
			Data d;
			FillTo(d, -reqAutoId, o);
			PEERTHIS->Send(std::move(d));
			co_await iter->second.first.async_wait(use_nothrow_awaitable);	// 等回包 或 超时
			auto r = std::move(iter->second.second);	// 拿出 网络回包
			reqs.erase(iter);	// 移除请求
			co_return r;	// 返回 网络回包( 超时 为 空 )
		}

		void SendPush(ObjBase_s const& o) {
			Data d;
			FillTo(d, 0, o);
			PEERTHIS->Send(std::move(d));
		}

		void SendResponse(int32_t serial, ObjBase_s const& o) {
			Data d;
			FillTo(d, serial, o);
			PEERTHIS->Send(std::move(d));
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
			if constexpr (hasTimeoutCode) {
				// 正在停止，直接吞掉所有数据
				if (PEERTHIS->stoping) return len;
			}

			// 取出指针备用
			auto buf = (uint8_t*)inBuf;
			auto end = (uint8_t*)inBuf + len;

			// 包头
			uint32_t dataLen = 0;
			//uint32_t addr = 0;	// todo

			// 确保包头长度充足
			while (buf + sizeof(dataLen) <= end) {
				// 取长度
				dataLen = *(uint32_t*)buf;

				// 计算包总长( 包头长 + 数据长 )
				auto totalLen = sizeof(dataLen) + dataLen;

				// 如果包不完整 就 跳出
				if (buf + totalLen > end) break;

				{
					auto dr = xx::Data_r(buf + sizeof(dataLen), dataLen);

					// todo: 网关包这里有个 投递地址 占用

					// 读出序号
					int32_t serial;
					if (dr.Read(serial)) return 0;

					// 如果是 Response 包，则在 req 字典查找。如果找到就 解包 + 传递 + 协程放行
					if (serial > 0) {
						if (auto iter = reqs.find(serial); iter != reqs.end()) {
							auto o = ReadFrom(dr);
							if (!o) return 0;
							iter->second.second = std::move(o);
							iter->second.first.cancel();
						}
					}
					else {
						// 如果是 Push 包，且有提供 ReceivePush 处理函数，就 解包 + 传递
						if (serial == 0) {
							if constexpr (HasReceivePush<PeerDeriveType>) {
								auto o = ReadFrom(dr);
								if (!o) return 0;
								if (PEERTHIS->ReceivePush(std::move(o))) return 0;
							}
						}
						// 如果是 Request 包，且有提供 ReceiveRequest 处理函数，就 解包 + 传递
						else {
							if constexpr (HasReceiveRequest<PeerDeriveType>) {
								auto o = ReadFrom(dr);
								if (!o) return 0;
								if (PEERTHIS->ReceiveRequest(-serial, std::move(o))) return 0;
							}
						}
						if (PEERTHIS->stoping || PEERTHIS->stoped) return 0;
					}
				}

				// 跳到下一个包的开头
				buf += totalLen;
			}

			// 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
			return buf - inBuf;
		}
	};

	// todo: more xxxxCode here

#undef PEERTHIS;
}
