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
		t.HandleMessage();		// int ( char*, len ) ����ֵΪ �Ѵ�����, ���� buf ���Ƴ�. ���� <= 0 ��ʾ������ Stop()
	};

	// ��Ա���� / ���̽����
	template<typename T> concept HasStart_ = requires(T t) { t.Start_(); };
	template<typename T> concept HasStop_ = requires(T t) { t.Stop_(); };

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
				((PeerDeriveType*)(this))->reqAutoId = 0;
			}
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Read(); }, asio::detached);
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Write(); }, asio::detached);
			if constexpr(HasStart_<PeerDeriveType>) {
				((PeerDeriveType*)(this))->Start_();
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
			if constexpr (HasStop_<PeerDeriveType>) {
				((PeerDeriveType*)(this))->Stop_();
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
			uint8_t buf[1024 * 256];
			size_t len = 0;
			for (;;) {
				auto [ec, n] = co_await socket.async_read_some(asio::buffer(buf + len, sizeof(buf) - len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (!n) continue;
				if constexpr (hasTimeoutCode) {
					if (((PeerDeriveType*)(this))->stoping) {
						len = 0;
						continue;
					}
				}
				len += n;
				n = ((PeerDeriveType*)(this))->HandleMessage(buf, len);
				if (stoped) co_return;
				if constexpr (hasTimeoutCode) {
					if (((PeerDeriveType*)(this))->stoping) co_return;
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

	template<typename ServerDeriveType>
	struct ServerCode : asio::noncopyable {
		asio::io_context ioc;				// .run() execute
		asio::signal_set signals;
		ServerCode() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

		// ��ҪĿ�� Peer ʵ��( Server&, &&socket ) ���캯��
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

	// Ϊ peer ���ӳ�ʱ�������
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

	// Ϊ peer ���� Send( Obj )( SendPush, SendRequest, SendResponse ) �� ��ع���
	/* ��Ҫ�ֹ�������к���
	int ReceiveRequest(xx::ObjBase_s&& o) {
	int ReceivePush(xx::ObjBase_s&& o) {
		// todo: handle o
		// �Ƿ� o ���ִ����� om.KillRecursive(o);
		return 0;	// Ҫ���߾ͷ��ط� 0
	}
	*/
	template<typename PeerDeriveType>
	struct PeerRequestCode {
		int32_t reqAutoId = 0;
		std::unordered_map<int32_t, std::pair<asio::steady_timer, ObjBase_s>> reqs;
		ObjManager& om;

		PeerRequestCode(ObjManager& om_) : om(om_) {}

		void FillTo(Data& d, int32_t const& serial, ObjBase_s const& o) {
			auto bak = d.WriteJump(sizeof(uint32_t));	// package len
			// todo: ���ذ������и� Ͷ�ݵ�ַ ռ��
			d.WriteVarInteger(serial);
			om.WriteTo(d, o);	// typeid + data
			d.WriteFixedAt(bak, (uint32_t)(d.len - sizeof(uint32_t)));	// fill package len
		}

		asio::awaitable<ObjBase_s> SendRequest(ObjBase_s const& o, std::chrono::steady_clock::duration timeoutSecs = 15s) {
			// ��������
			reqAutoId = (reqAutoId + 1) % 0x7fffffff;
			auto iter = reqs.emplace(reqAutoId, std::make_pair(asio::steady_timer(((PeerDeriveType*)(this))->ioc, std::chrono::steady_clock::now() + timeoutSecs), ObjBase_s())).first;
			Data d;
			FillTo(d, -reqAutoId, o);
			((PeerDeriveType*)(this))->Send(std::move(d));
			co_await iter->second.first.async_wait(use_nothrow_awaitable);	// �Ȼذ� �� ��ʱ
			auto r = std::move(iter->second.second);	// �ó� ����ذ�
			reqs.erase(iter);	// �Ƴ�����
			co_return r;	// ���� ����ذ�( ��ʱ Ϊ �� )
		}

		void SendPush(ObjBase_s const& o) {
			Data d;
			FillTo(d, 0, o);
			((PeerDeriveType*)(this))->Send(std::move(d));
		}

		void SendResponse(int32_t serial, ObjBase_s const& o) {
			Data d;
			FillTo(d, serial, o);
			((PeerDeriveType*)(this))->Send(std::move(d));
		}

		size_t HandleMessage(uint8_t* inBuf, size_t len) {
			// ����ֹͣ��ֱ���̵���������
			if (((PeerDeriveType*)(this))->stoping) return len;

			// ȡ��ָ�뱸��
			auto buf = (uint8_t*)inBuf;
			auto end = (uint8_t*)inBuf + len;

			// ��ͷ
			uint32_t dataLen = 0;
			//uint32_t addr = 0;	// todo

			// ȷ����ͷ���ȳ���
			while (buf + sizeof(dataLen) <= end) {
				// ȡ����
				dataLen = *(uint32_t*)buf;

				// ������ܳ�( ��ͷ�� + ���ݳ� )
				auto totalLen = sizeof(dataLen) + dataLen;

				// ����������� �� ����
				if (buf + totalLen > end) break;

				{
					auto dr = xx::Data_r(buf + sizeof(dataLen), dataLen);

					// todo: ���ذ������и� Ͷ�ݵ�ַ ռ��

					int32_t serial;
					if (dr.Read(serial)) return 0;

					xx::ObjBase_s o;
					if (om.ReadFrom(dr, o)) {
						om.KillRecursive(o);
						return 0;
					}
					if (!o || !o.typeId()) return 0;

					if (serial > 0) {
						if (auto iter = reqs.find(serial); iter != reqs.end()) {
							iter->second.second = std::move(o);
							iter->second.first.cancel();
						}
					}
					else {
						if (serial == 0) {
							if (((PeerDeriveType*)(this))->ReceivePush(std::move(o))) return 0;
						}
						else {	// serial < 0
							if (((PeerDeriveType*)(this))->ReceiveRequest(-serial, std::move(o))) return 0;
						}
						if (((PeerDeriveType*)(this))->stoping || ((PeerDeriveType*)(this))->stoped) return 0;
					}
				}

				// ������һ�����Ŀ�ͷ
				buf += totalLen;
			}

			// �Ƴ����Ѵ��������( ������ʣ�µ������ƶ���ͷ�� )
			return buf - inBuf;
		}
	};

	// todo: more xxxxCode here
}
