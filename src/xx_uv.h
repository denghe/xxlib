#pragma once
#include "xx_helpers.h"
#include "xx_data_funcs.h"
#include "xx_dict.h"
#include "xx_obj.h"
#include "ikcp.h"
#include "uv.h"
#include <mutex>
#include <deque>

namespace xx {
	struct UvKcp;
	struct Uv {
		int64_t defaultRequestTimeoutMS = 15000;	// for SendRequest( .... , 0 )
		uint32_t maxPackageLength = 1024 * 256;		// for recv safe check

		ObjManager om;
		Data recvBB;								// shared deserialization for package receive. direct replace buf when using
		Data sendBB;								// shared serialization for package send

		int autoId = 0;								// udps key, udp dialer port gen: --autoId
		Dict<int, Weak<UvKcp>> udps;		// key: port( dialer peer port = autoId )
		char* recvBuf = nullptr;					// shared receive buf for kcp
		size_t recvBufLen = 65535;					// shared receive buf's len
		uv_run_mode runMode = UV_RUN_DEFAULT;		// reduce frame client update kcp delay
		uv_loop_t uvLoop;

		Uv() {
			if (int r = uv_loop_init(&uvLoop)) throw r;
			recvBuf = new char[recvBufLen];
		}
		Uv(Uv const&) = delete;
		Uv& operator=(Uv const&) = delete;

		~Uv() {
			recvBB.Reset();							// unbind buf( do not need free )
			if (recvBuf) {
				delete[] recvBuf;
				recvBuf = nullptr;
			}

			int r = uv_run(&uvLoop, UV_RUN_DEFAULT);
			assert(!r);
			r = uv_loop_close(&uvLoop);
			assert(!r);
			(void)r;
		}

		inline int Run(uv_run_mode const& mode = UV_RUN_DEFAULT) noexcept {
			runMode = mode;
			return uv_run(&uvLoop, mode);
		}

		inline void Stop() {
			uv_stop(&uvLoop);
		}

		template<typename Func>
		void DelayExec(uint64_t ms, Func&& func);

		template<typename T>
		static T* Alloc(void* const& ud) noexcept {
			auto p = (void**)::malloc(sizeof(void*) + sizeof(T));
			if (!p) return nullptr;
			p[0] = ud;
			return (T*)& p[1];
		}
		inline static void Free(void* const& p) noexcept {
			::free((void**)p - 1);
		}
		template<typename T>
		static T* GetSelf(void* const& p) noexcept {
			return (T*) * ((void**)p - 1);
		}
		template<typename T>
		static void HandleCloseAndFree(T*& tar) noexcept {
			if (!tar) return;
			auto h = (uv_handle_t*)tar;
			tar = nullptr;
			assert(!uv_is_closing(h));
			uv_close(h, [](uv_handle_t* handle) {
				Uv::Free(handle);
				});
		}
		inline static void AllocCB(uv_handle_t* h, size_t suggested_size, uv_buf_t* buf) noexcept {
			buf->base = (char*)::malloc(suggested_size);
			buf->len = decltype(uv_buf_t::len)(suggested_size);
		}

		inline static int FillIP(sockaddr_in6& saddr, std::string& ip, bool includePort = true) noexcept {
			ip.resize(64);
			if (saddr.sin6_family == AF_INET6) {
				if (int r = uv_ip6_name(&saddr, (char*)ip.data(), ip.size())) return r;
				ip.resize(strlen(ip.data()));
				if (includePort) {
					ip.append(":");
					ip.append(std::to_string(ntohs(saddr.sin6_port)));
				}
			}
			else {
				if (int r = uv_ip4_name((sockaddr_in*)& saddr, (char*)ip.data(), ip.size())) return r;
				ip.resize(strlen(ip.data()));
				if (includePort) {
					ip.append(":");
					ip.append(std::to_string(ntohs(((sockaddr_in*)& saddr)->sin_port)));
				}
			}
			return 0;
		}
		inline static int FillIP(uv_tcp_t* stream, std::string& ip, bool includePort = true) noexcept {
			sockaddr_in6 saddr;
			int len = sizeof(saddr);
			int r = 0;
			if ((r = uv_tcp_getpeername(stream, (sockaddr*)& saddr, &len))) return r;
			return FillIP(saddr, ip, includePort);
		}
		inline static std::string ToIpPortString(sockaddr const* const& addr, bool includePort = true) noexcept {
			sockaddr_in6 a;
			memcpy(&a, addr, sizeof(a));
			std::string ipAndPort;
			Uv::FillIP(a, ipAndPort, includePort);
			return ipAndPort;
		}
		inline static std::string ToIpPortString(sockaddr_in6 const& addr, bool includePort = true) noexcept {
			return ToIpPortString((sockaddr*)& addr, includePort);
		}
	};

	struct UvItem {
		Uv& uv;
		UvItem(Uv& uv) : uv(uv) {}
		virtual ~UvItem() {/* this->Dispose(-1); */ }

		virtual bool Disposed() const noexcept = 0;

		// flag == -1: call by destructor.  0 : do not callback.  1: callback
		// return true: dispose success. false: already disposed.
		virtual bool Dispose(int const& flag = 1) noexcept = 0;

		// user data
		void* userData = nullptr;
	};
	using UvItem_s = Shared<UvItem>;
	using UvItem_w = Weak<UvItem>;

	struct UvAsync : UvItem {
		uv_async_t* uvAsync = nullptr;
		std::mutex mtx;
		std::deque<std::function<void()>> actions;
		std::function<void()> action;	// for pop store

		UvAsync(Uv& uv)
			: UvItem(uv) {
			uvAsync = Uv::Alloc<uv_async_t>(this);
			if (!uvAsync) throw - 1;
			if (int r = uv_async_init(&uv.uvLoop, uvAsync, [](uv_async_t* handle) {
				Uv::GetSelf<UvAsync>(handle)->Execute();
				})) {
				uvAsync = nullptr;
				throw r;
			}
		}
		UvAsync(UvAsync const&) = delete;
		UvAsync& operator=(UvAsync const&) = delete;
		~UvAsync() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return !uvAsync;
		}
		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!uvAsync) return false;
			Uv::HandleCloseAndFree(uvAsync);
			if (flag == -1) return true;
			auto holder = SharedFromThis(this);
			actions.clear();
			return true;
		}
		inline int Dispatch(std::function<void()>&& action) noexcept {
			if (!uvAsync) return -1;
			{
				std::scoped_lock<std::mutex> g(mtx);
				actions.push_back(std::move(action));
			}
			return uv_async_send(uvAsync);
		}

	protected:
		inline void Execute() noexcept {
			{
				std::scoped_lock<std::mutex> g(mtx);
				action = std::move(actions.front());
				actions.pop_front();
			}
			action();
		}
	};
	using UvAsync_s = Shared<UvAsync>;
	using UvAsync_w = Weak<UvAsync>;

	struct UvTimer : UvItem {
		uv_timer_t* uvTimer = nullptr;
		uint64_t timeoutMS = 0;
		uint64_t repeatIntervalMS = 0;
		std::function<void()> onFire;

		UvTimer(Uv& uv)
			: UvItem(uv) {
			uvTimer = Uv::Alloc<uv_timer_t>(this);
			if (!uvTimer) throw - 1;
			if (int r = uv_timer_init(&uv.uvLoop, uvTimer)) {
				uvTimer = nullptr;
				throw r;
			}
		}
		UvTimer(Uv& uv, uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr)
			: UvTimer(uv) {
			if (int r = Start(timeoutMS, repeatIntervalMS, std::move(onFire))) throw r;
		}
		UvTimer(UvTimer const&) = delete;
		UvTimer& operator=(UvTimer const&) = delete;
		~UvTimer() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return !uvTimer;
		}
		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!uvTimer) return false;
			Uv::HandleCloseAndFree(uvTimer);
			if (flag == -1) return true;
			auto holder = SharedFromThis(this);	// if not, gcc will crash at random place
			onFire = nullptr;
			return true;
		}

		inline int Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS, std::function<void()>&& onFire = nullptr) noexcept {
			if (!uvTimer) return -1;
			this->timeoutMS = timeoutMS;
			this->repeatIntervalMS = repeatIntervalMS;
			this->onFire = std::move(onFire);
			return uv_timer_start(uvTimer, Fire, timeoutMS, repeatIntervalMS);
		}
		inline int Restart() noexcept {
			if (!uvTimer) return -1;
			return uv_timer_start(uvTimer, Fire, timeoutMS, repeatIntervalMS);
		}
		inline int Stop() noexcept {
			if (!uvTimer) return -1;
			return uv_timer_stop(uvTimer);
		}
		inline int Again() noexcept {
			if (!uvTimer) return -1;
			return uv_timer_again(uvTimer);
		}

		// force the loop to exit early by unreferencing handles which are active
		inline void Unref() noexcept {
			uv_unref((uv_handle_t*)uvTimer);
		}

	protected:
		inline static void Fire(uv_timer_t* t) {
			auto self = Uv::GetSelf<UvTimer>(t);
			if (self->onFire) {
				self->onFire();
			}
		}
	};
	using UvTimer_s = Shared<UvTimer>;
	using UvTimer_w = Weak<UvTimer>;



	template<typename Func>
	void Uv::DelayExec(uint64_t ms, Func&& func) {
		auto&& t = xx::Make<UvTimer>(*this);
		t->Start(ms, 0, [t, func = std::forward<Func>(func)]{
			func();
			t->Dispose();
			});
	}



	struct UvResolver;
	using UvResolver_s = Shared<UvResolver>;
	using UvResolver_w = Weak<UvResolver>;
	struct uv_getaddrinfo_t_ex {
		uv_getaddrinfo_t req;
		UvResolver_w resolver_w;
	};

	struct UvResolver : UvItem {
		uv_getaddrinfo_t_ex* req = nullptr;
		UvTimer_s timeouter;
		bool disposed = false;
		std::vector<std::string> ips;
		std::function<void()> onFinish;
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
		addrinfo hints;
#endif

		UvResolver(Uv& uv) noexcept
			: UvItem(uv) {
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
			hints.ai_family = PF_UNSPEC;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = 0;// IPPROTO_TCP;
			hints.ai_flags = AI_DEFAULT;
#endif
		}

		UvResolver(UvResolver const&) = delete;
		UvResolver& operator=(UvResolver const&) = delete;
		~UvResolver() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return disposed;
		}
		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (disposed) return false;
			Cancel();
			if (flag == -1) return true;
			auto holder = SharedFromThis(this);
			onFinish = nullptr;
			return true;
		}

		inline void Cancel() {
			if (disposed) return;
			ips.clear();
			if (req) {
				uv_cancel((uv_req_t*)req);
				req = nullptr;
			}
			timeouter.Reset();
		}

		inline bool Busy() {
			return (bool)timeouter;
		}

		inline int Resolve(std::string const& domainName, uint64_t const& timeoutMS = 3000) noexcept {
			if (disposed) return -1;
			if (!timeoutMS) return -2;
			Cancel();
			TryMakeTo(timeouter, uv, timeoutMS, 0, [this] {
				auto self = this;
				Cancel();
				if (self->onFinish) {
					self->onFinish();
				}
				});
			if (!timeouter) return -2;
			auto req = std::make_unique<uv_getaddrinfo_t_ex>();
			req->resolver_w = SharedFromThis(this);
			if (int r = uv_getaddrinfo((uv_loop_t*)& uv.uvLoop, (uv_getaddrinfo_t*)& req->req, [](uv_getaddrinfo_t* req_, int status, struct addrinfo* ai) {
				auto req = std::unique_ptr<uv_getaddrinfo_t_ex>(container_of(req_, uv_getaddrinfo_t_ex, req));
				if (status) return;													// error or -4081 canceled
				auto resolver = req->resolver_w.Lock();
				if (!resolver) return;
				resolver->req = nullptr;
				assert(ai);

				auto& ips = resolver->ips;
				std::string s;
				do {
					s.resize(64);
					if (ai->ai_addr->sa_family == AF_INET6) {
						uv_ip6_name((sockaddr_in6*)ai->ai_addr, (char*)s.data(), s.size());
					}
					else {
						uv_ip4_name((sockaddr_in*)ai->ai_addr, (char*)s.data(), s.size());
					}
					s.resize(strlen(s.data()));

					if (std::find(ips.begin(), ips.end(), s) == ips.end()) {
						ips.push_back(std::move(s));								// known issue: ios || android will receive duplicate result
					}
					ai = ai->ai_next;
				} while (ai);
				uv_freeaddrinfo(ai);

				resolver->timeouter.Reset();
				if (resolver->onFinish) {
					resolver->onFinish();
				}

				}, domainName.c_str(), nullptr,
#ifdef __IPHONE_OS_VERSION_MIN_REQUIRED
				(const addrinfo*) & hints
#else
					nullptr
#endif
					)) return r;
			this->req = req.release();
			return 0;
		}
	};

	struct UvPeer;
	using UvPeer_s = Shared<UvPeer>;
	using UvPeer_w = Weak<UvPeer>;

	struct UvPeerBase : UvItem {
		using UvItem::UvItem;
		UvPeer* peer = nullptr;
		virtual std::string GetIP() noexcept = 0;

		virtual int SendDirect(uint8_t* const& buf, size_t const& len) noexcept = 0;		// direct send anything
		virtual void SendPrepare(Data& bb, size_t const& reserveLen) noexcept = 0;		// resize ctx & header space to bb
		virtual int SendAfterPrepare(Data& bb) noexcept = 0;								// fill ctx & header & send

		virtual void Flush() noexcept = 0;
		virtual int Update(int64_t const& nowMS) noexcept = 0;
		virtual bool IsKcp() noexcept = 0;
	};
	using UvPeerBase_s = Shared<UvPeerBase>;

	struct UvCreateAcceptBase : UvItem {
		using UvItem::UvItem;
		std::function<UvPeer_s(Uv& uv)> onCreatePeer;
		std::function<void(UvPeer_s peer)> onAccept;

		virtual UvPeer_s CreatePeer() noexcept;
		virtual void Accept(UvPeer_s peer) noexcept;
	};

	struct UvKcpListener;
	struct UvTcpListener;
	struct UvListener : UvCreateAcceptBase {
		Shared<UvTcpListener> tcpListener;
		Shared<UvKcpListener> kcpListener;

		// tcpKcpOpt == 0: tcp     == 1: kcp      == 2: both
		UvListener(Uv& uv, std::string const& ip, int const& port, int const& tcpKcpOpt);
		~UvListener() { this->Dispose(-1); }
		inline virtual bool Disposed() const noexcept override {
			return !tcpListener && !kcpListener;
		}
		virtual bool Dispose(int const& flag = 1) noexcept override;
	};
	using UvListener_s = Shared<UvListener>;
	using UvListener_w = Weak<UvListener>;


	struct UvListenerBase : UvItem {
		using UvItem::UvItem;
		UvListener* listener = nullptr;
		virtual void Accept(UvPeerBase_s peer) noexcept;
	};

	struct UvDialer;
	struct UvDialerBase : UvItem {
		using UvItem::UvItem;
		UvDialer* dialer = nullptr;
		bool disposed = false;
		inline virtual bool Disposed() const noexcept override {
			return disposed;
		}
		virtual int Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS = 0, bool cleanup = true) noexcept = 0;
		virtual int Dial(std::vector<std::string> const& ips, int const& port, uint64_t const& timeoutMS) noexcept;
		virtual int Dial(std::vector<std::pair<std::string, int>> const& ipports, uint64_t const& timeoutMS) noexcept;
		virtual void Accept(UvPeerBase_s pb) noexcept;
		virtual void Cancel() noexcept = 0;
	};

	struct UvPeer : UvItem {
		UvPeerBase_s peerBase;		// fill after make
		Dict<int, std::pair<std::function<int(ObjBase_s&& msg)>, int64_t>> callbacks;
		int serial = 0;
		int64_t timeoutMS = 0;
		UvTimer_s timer;
		std::function<void()> onDisconnect;
		std::function<int(ObjBase_s&& msg)> onReceivePush;
		std::function<int(int const& serial, ObjBase_s&& msg)> onReceiveRequest;
		std::string ip;			// cache

		UvPeer(Uv& uv)
			: UvItem(uv) {
			MakeTo(timer, uv, 10, 10, [this] {
				Update(NowSteadyEpochMilliseconds());
				});
		}

		inline std::string GetIP() noexcept {
			if (!peerBase || ip.size()) return ip;
			return ip = peerBase->GetIP();
		}

		inline bool IsKcp() noexcept {
			if (!peerBase) return false;
			return peerBase->IsKcp();
		}

		inline void ResetTimeoutMS(int64_t const& ms) noexcept {
			timeoutMS = ms ? NowSteadyEpochMilliseconds() + ms : 0;
		}

		inline void Flush() noexcept {
			peerBase->Flush();
		};

		inline virtual void Disconnect() noexcept {
			if (onDisconnect) {
				onDisconnect();
			}
		}

		inline virtual int ReceivePush(ObjBase_s&& msg) noexcept {
			return onReceivePush ? onReceivePush(std::move(msg)) : 0;
		}

		inline virtual int ReceiveRequest(int const& serial, ObjBase_s&& msg) noexcept {
			return onReceiveRequest ? onReceiveRequest(serial, std::move(msg)) : 0;
		}

		inline int SendDirect(uint8_t* const& buf, size_t const& len) noexcept {
			return peerBase->SendDirect(buf, len);
		}

		inline int SendPush(ObjBase_s const& msg) noexcept {
			return SendResponse(0, msg);
		}

		inline int SendResponse(int32_t const& serial, ObjBase_s const& msg) noexcept {
			auto&& bb = uv.sendBB;
			peerBase->SendPrepare(bb, 1024);
			bb.WriteVarInteger(serial);
			uv.om.WriteTo(bb, msg);
			return peerBase->SendAfterPrepare(bb);
		}

		inline int SendRequest(ObjBase_s const& msg, std::function<int(ObjBase_s&& msg)>&& cb, uint64_t const& timeoutMS) noexcept {
			if (!peerBase) return -1;
			std::pair<std::function<int(ObjBase_s && msg)>, int64_t> v;
			serial = (serial + 1) & 0x7FFFFFFF;			// uint circle use
			v.second = NowSteadyEpochMilliseconds() + (timeoutMS ? timeoutMS : uv.defaultRequestTimeoutMS);
			if (int r = SendResponse(-serial, msg)) return r;
			v.first = std::move(cb);
			callbacks[serial] = std::move(v);
			return 0;
		}

		inline virtual int HandlePack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			// for kcp listener accept
			if (recvLen == 1 && *recvBuf == 0) {
				ip = peerBase->GetIP();
				return 0;
			}

			auto& bb = uv.recvBB;
			bb.Reset((uint8_t*)recvBuf, recvLen);

			int serial = 0;
			if (int r = bb.ReadVarInteger(serial)) return r;

			ObjBase_s msg;
			if (int r = uv.om.ReadFrom(bb, msg)) return r;

			if (serial == 0) {
				return ReceivePush(std::move(msg));
			}
			else if (serial < 0) {
				return ReceiveRequest(-serial, std::move(msg));
			}
			else {
				auto&& idx = callbacks.Find(serial);
				if (idx == -1) return 0;
				auto a = std::move(callbacks.ValueAt(idx).first);
				callbacks.RemoveAt(idx);
				return a(std::move(msg));
			}
		}

		// call by timer
		inline virtual int Update(int64_t const& nowMS) noexcept {
			assert(peerBase);
			if (timeoutMS && timeoutMS < nowMS) {
				Dispose();
				return -1;
			}

			if (int r = peerBase->Update(nowMS)) return r;

			for (auto&& iter = callbacks.begin(); iter != callbacks.end(); ++iter) {
				auto&& v = iter->value;
				if (v.second < nowMS) {
					auto a = std::move(v.first);
					iter.Remove();
					a(nullptr);
				}
			}

			return 0;
		}

		inline virtual bool Disposed() const noexcept override {
			return !peerBase;
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!peerBase) return false;
			peerBase.Reset();
			timer.Reset();
			for (auto&& kv : callbacks) {
				kv.value.first(nullptr);
			}
			callbacks.Clear();
			if (flag == -1) return true;
			auto holder = SharedFromThis(this);
			if (flag == 1) {
				Disconnect();
			}
			onDisconnect = nullptr;
			onReceivePush = nullptr;
			onReceiveRequest = nullptr;
			return true;
		}
	};

	inline UvPeer_s UvCreateAcceptBase::CreatePeer() noexcept {
		return onCreatePeer ? onCreatePeer(uv) : TryMake<UvPeer>(uv);
	}

	inline void UvCreateAcceptBase::Accept(UvPeer_s peer) noexcept {
		if (onAccept) {
			assert(!peer || peer->peerBase);
			onAccept(peer);
		}
	}

	inline void UvListenerBase::Accept(UvPeerBase_s pb) noexcept {
		assert(pb);
		auto&& p = listener->CreatePeer();
		if (!p) return;
		p->peerBase = pb;
		pb->peer = &*p;
		listener->Accept(p);
	}

	struct uv_write_t_ex : uv_write_t {
		uv_buf_t buf;
	};

	struct UvTcpPeerBase : UvPeerBase {
		uv_tcp_t* uvTcp = nullptr;
		std::string ip;
		Data buf;

		UvTcpPeerBase(Uv& uv) : UvPeerBase(uv) {
			uvTcp = Uv::Alloc<uv_tcp_t>(this);
			if (!uvTcp) throw - 1;
			if (int r = uv_tcp_init(&uv.uvLoop, uvTcp)) {
				uvTcp = nullptr;
				throw r;
			}
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!uvTcp) return false;
			Uv::HandleCloseAndFree(uvTcp);
			if (flag != -1) {
				peer->Dispose(flag);
			}
			return true;
		}

		~UvTcpPeerBase() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return !uvTcp;
		}

		inline virtual bool IsKcp() noexcept override {
			return false;
		}

		inline virtual std::string GetIP() noexcept override {
			return ip;
		}

		inline virtual int SendDirect(uint8_t* const& buf, size_t const& len) noexcept override {
			auto&& bb = uv.sendBB;
			bb.Reserve(sizeof(uv_write_t_ex) + len);
			bb.len = sizeof(uv_write_t_ex);
			auto&& req = *(uv_write_t_ex*)bb.buf;
			req.buf.base = (char*)buf;										// fill req.buf
			req.buf.len = decltype(uv_buf_t::len)(len);
			bb.Reset();														// unbind bb.buf for callback ::free(req)
			if (int r = uv_write(&req, (uv_stream_t*)uvTcp, &req.buf, 1, [](uv_write_t* req, int status) { ::free(req); })) {
				Dispose();
				return r;
			}
			return 0;
		}

		inline virtual void SendPrepare(Data& bb, size_t const& reserveLen) noexcept override {
			bb.Reserve(sizeof(uv_write_t_ex) + 4 + reserveLen);
			bb.len = sizeof(uv_write_t_ex) + 4;		// skip header space
		}
		inline virtual int SendAfterPrepare(Data& bb) noexcept {
			auto buf = bb.buf + sizeof(uv_write_t_ex);						// ref to header
			auto len = (uint32_t)(bb.len - sizeof(uv_write_t_ex) - 4);		// calc data's len
			::memcpy(buf, &len, sizeof(len));								// fill header
			auto&& req = *(uv_write_t_ex*)bb.buf;
			req.buf.base = (char*)buf;										// fill req.buf
			req.buf.len = decltype(uv_buf_t::len)(len + 4);					// send len = data's len + header's len
			bb.Reset();														// unbind bb.buf for callback ::free(req)
			if (int r = uv_write(&req, (uv_stream_t*)uvTcp, &req.buf, 1, [](uv_write_t* req, int status) { ::free(req); })) {
				Dispose();
				return r;
			}
			return 0;
		}

		inline virtual void Flush() noexcept override {}

		inline virtual int Update(int64_t const& nowMS) noexcept override { return 0; }


		// called by dialer or listener
		inline int ReadStart() noexcept {
			if (!uvTcp) return -1;
			return uv_read_start((uv_stream_t*)uvTcp, Uv::AllocCB, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
				auto self = Uv::GetSelf<UvTcpPeerBase>(stream);
				auto holder = SharedFromThis(self);	// hold for callback Dispose
				if (nread > 0) {
					nread = self->Unpack((uint8_t*)buf->base, (uint32_t)nread);
				}
				if (buf) ::free(buf->base);
				if (nread < 0) {
					self->Dispose();
				}
				});
		}

		// 4 byte len header. can override for write custom header format
		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			buf.WriteBuf(recvBuf, recvLen);
			size_t offset = 0;
			while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
				uint32_t len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
				if (len > uv.maxPackageLength) return -1;			// invalid length
				if (offset + 4 + len > buf.len) break;				// not enough data

				offset += 4;
				if (int r = peer->HandlePack(buf.buf + offset, len)) return r;
				offset += len;
			}
			buf.RemoveFront(offset);
			return 0;
		}
	};

	struct uv_udp_send_t_ex : uv_udp_send_t {
		uv_buf_t buf;
	};

	struct UvKcp : UvItem {
		uv_udp_t* uvUdp = nullptr;
		sockaddr_in6 addr;
		Data buf;
		int port = 0;								// fill by owner. dict's key. port > 0: listener  < 0: dialer fill by --uv.udpId
		virtual void Remove(uint32_t const& conv) noexcept = 0;

		UvKcp(Uv& uv, std::string const& ip, int const& port, bool const& isListener)
			: UvItem(uv) {
			if (ip.size()) {
				if (ip.find(':') == std::string::npos) {
					if (int r = uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)& addr)) throw r;
				}
				else {
					if (int r = uv_ip6_addr(ip.c_str(), port, &addr)) throw r;
				}
			}
			uvUdp = Uv::Alloc<uv_udp_t>(this);
			if (!uvUdp) throw - 2;
			if (int r = uv_udp_init(&uv.uvLoop, uvUdp)) {
				uvUdp = nullptr;
				throw r;
			}
			auto sgUdp = xx::MakeScopeGuard([this] { Uv::HandleCloseAndFree(uvUdp); });
			if (isListener) {
				if (int r = uv_udp_bind(uvUdp, (sockaddr*)& addr, UV_UDP_REUSEADDR)) throw r;
			}
			if (int r = uv_udp_recv_start(uvUdp, Uv::AllocCB, [](uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
				auto self = Uv::GetSelf<UvKcp>(handle);
				auto holder = SharedFromThis(self);	// hold for callback Dispose
				if (nread > 0) {
					nread = self->Unpack((uint8_t*)buf->base, (uint32_t)nread, addr);
				}
				if (buf) ::free(buf->base);
				if (nread < 0) {
					self->Dispose();
				}
				})) throw r;
			sgUdp.Cancel();
		}
		UvKcp(UvKcp const&) = delete;
		UvKcp& operator=(UvKcp const&) = delete;
		~UvKcp() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return !uvUdp;
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!uvUdp) return false;
			Uv::HandleCloseAndFree(uvUdp);
			return true;
		}

		// send target: addr or this->addr
		inline virtual int Send(uint8_t const* const& buf, ssize_t const& dataLen, sockaddr const* const& addr = nullptr) noexcept {
			if (!uvUdp) return -1;
			auto req = (uv_udp_send_t_ex*)::malloc(sizeof(uv_udp_send_t_ex) + dataLen);
			if (!req) return -2;
			memcpy(req + 1, buf, dataLen);
			req->buf.base = (char*)(req + 1);
			req->buf.len = decltype(uv_buf_t::len)(dataLen);
			// todo: check send queue len ? protect?
			if (int r = uv_udp_send(req, uvUdp, &req->buf, 1, addr ? addr : (sockaddr*)& this->addr, [](uv_udp_send_t* req, int status) { ::free(req); })) {
				Dispose();
				return r;
			}
			return 0;
		}

	protected:
		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept = 0;
	};

	struct UvKcpPeerBase : UvPeerBase {
		using UvPeerBase::UvPeerBase;
		Shared<UvKcp> udp;					// fill by creater
		uint32_t conv = 0;							// fill by creater
		int64_t createMS = 0;						// fill by creater
		ikcpcb* kcp = nullptr;
		uint32_t nextUpdateMS = 0;					// for kcp update interval control. reduce cpu usage
		Data buf;
		sockaddr_in6 addr;							// for Send. fill by owner Unpack

		// require: fill udp, conv, createMS, addr
		inline int InitKcp() {
			if (kcp) return -1;
			assert(conv);
			kcp = ikcp_create(conv, this);
			if (!kcp) return -1;
			auto sgKcp = xx::MakeScopeGuard([&] { ikcp_release(kcp); kcp = nullptr; });
			if (int r = ikcp_wndsize(kcp, 1024, 1024)) return r;
			if (int r = ikcp_nodelay(kcp, 1, 10, 2, 1)) return r;
			kcp->rx_minrto = 10;
			kcp->stream = 1;
			ikcp_setoutput(kcp, [](const char* inBuf, int len, ikcpcb* kcp, void* user)->int {
				auto self = ((UvKcpPeerBase*)user);
				return self->udp->Send((uint8_t*)inBuf, len, (sockaddr*)& self->addr);
				});
			sgKcp.Cancel();
			return 0;
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!kcp) return false;
			ikcp_release(kcp);
			kcp = nullptr;
			udp->Remove(conv);						// remove self from container
			udp.Reset();							// unbind
			if (flag != -1) {
				peer->Dispose(flag);
			}
			return true;
		}

		~UvKcpPeerBase() { this->Dispose(-1); }

		inline virtual bool IsKcp() noexcept override {
			return true;
		}

		inline virtual std::string GetIP() noexcept override {
			std::string ip;
			Uv::FillIP(addr, ip);
			return ip;
		}

		virtual bool Disposed() const noexcept override {
			return !kcp;
		}

		// called by ext class
		inline virtual int Update(int64_t const& nowMS) noexcept override {
			if (!kcp) return -1;

			auto&& currentMS = uint32_t(nowMS - createMS);			// known issue: uint32 limit. connect only alive 50+ days
			if (uv.runMode == UV_RUN_DEFAULT && nextUpdateMS > currentMS) return 0;			// reduce cpu usage
			ikcp_update(kcp, currentMS);
			if (!kcp) return -1;
			if (uv.runMode == UV_RUN_DEFAULT) {
				nextUpdateMS = ikcp_check(kcp, currentMS);
			}

			do {
				int recvLen = ikcp_recv(kcp, uv.recvBuf, (int)uv.recvBufLen);
				if (recvLen <= 0) break;
				if (int r = Unpack((uint8_t*)uv.recvBuf, recvLen)) {
					Dispose();
					return r;
				}
			} while (true);

			return 0;
		}

		inline virtual int SendDirect(uint8_t* const& buf, size_t const& len) noexcept override {
			return Send(buf, len);
		}

		inline virtual void SendPrepare(Data& bb, size_t const& reserveLen) noexcept override {
			bb.Reserve(4 + reserveLen);
			bb.len = 4;												// skip header space
		}

		inline virtual int SendAfterPrepare(Data& bb) noexcept {
			auto len = uint32_t(bb.len - 4);						// calc data's len
			memcpy(bb.buf, &len, 4);								// fill header
			return Send(bb.buf, bb.len);							// send by kcp
		}

		// push send data to kcp. though ikcp_setoutput func send.
		inline int Send(uint8_t const* const& buf, ssize_t const& dataLen) noexcept {
			if (!kcp) return -1;
			return ikcp_send(kcp, (char*)buf, (int)dataLen);
		}

		// send data immediately ( no wait for more data combine send )
		inline virtual void Flush() noexcept override {
			if (!kcp) return;
			ikcp_flush(kcp);
		}

		// called by udp class. put data to kcp when udp receive 
		inline int Input(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			if (!kcp) return -1;
			return ikcp_input(kcp, (char*)recvBuf, recvLen);
		}

		// 4 bytes len header. can override for custom header format.
		inline virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen) noexcept {
			buf.WriteBuf(recvBuf, recvLen);
			size_t offset = 0;
			while (offset + 4 <= buf.len) {							// ensure header len( 4 bytes )
				uint32_t len = buf[offset + 0] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24);
				if (len > uv.maxPackageLength) return -1;			// invalid length
				if (offset + 4 + len > buf.len) break;				// not enough data

				offset += 4;
				if (int r = peer->HandlePack(buf.buf + offset, len)) return r;
				offset += len;
			}
			buf.RemoveFront(offset);
			return 0;
		}
	};

	struct UvListenerKcp : UvKcp {
		using UvKcp::UvKcp;
		UvListenerBase* owner = nullptr;			// fill by owner
		Dict<uint32_t, Weak<UvKcpPeerBase>> peers;
		Dict<std::string, std::pair<uint32_t, int64_t>> shakes;	// key: ip:port   value: conv, nowMS
		uint32_t convId = 0;
		int handShakeTimeoutMS = 3000;
		UvTimer_s timer;

		UvListenerKcp(Uv& uv, std::string const& ip, int const& port, bool const& isListener)
			: UvKcp(uv, ip, port, isListener) {
			MakeTo(timer, uv, 10, 10, [this] {
				this->Update(NowSteadyEpochMilliseconds());
				});
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!this->UvKcp::Dispose(0)) return false;
			for (auto&& kv : peers) {
				if (auto && peer = kv.value.Lock()) {
					peer->Dispose(flag == -1 ? 0 : 1);
				}
			}
			peers.Clear();
			uv.udps.Remove(port);
			return true;
		}

		~UvListenerKcp() { this->Dispose(-1); }

		inline virtual void Update(int64_t const& nowMS) noexcept {
			for (auto&& kv : peers) {
				assert(kv.value.Lock());
				kv.value.Lock()->Update(nowMS);
			}
			for (auto&& iter = shakes.begin(); iter != shakes.end(); ++iter) {
				if (iter->value.second < nowMS) {
					iter.Remove();
				}
			}
		}

		inline virtual void Remove(uint32_t const& conv) noexcept override {
			peers.Remove(conv);
		}

	protected:
		inline virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
			assert(port);
			// ensure hand shake, and udp peer owner still alive
			if (recvLen == 4 && owner) {						// hand shake contain 4 bytes auto inc serial
				auto&& ipAndPort = Uv::ToIpPortString(addr);
				// ip_port : <conv, createMS>
				auto&& idx = shakes.Find(ipAndPort);
				if (idx == -1) {
					idx = shakes.Add(ipAndPort, std::make_pair(++convId, NowSteadyEpochMilliseconds() + handShakeTimeoutMS)).index;
				}
				memcpy(recvBuf + 4, &shakes.ValueAt(idx).first, 4);	// return serial + conv( temp write to recvBuf is safe )
				return this->Send(recvBuf, 8, addr);
			}

			// header size limit IKCP_OVERHEAD ( kcp header ) check. ignore non kcp or hand shake pkgs.
			if (recvLen < 24) {
				return 0;
			}

			// read conv header
			uint32_t conv;
			memcpy(&conv, recvBuf, sizeof(conv));
			Shared<UvKcpPeerBase> peer;

			// find at peers. if does not exists, find addr at shakes. if exists, create peer
			auto&& peerIter = peers.Find(conv);
			if (peerIter == -1) {								// conv not found: scan shakes
				if (!owner || owner->Disposed()) return 0;		// listener disposed: ignore
				auto&& ipAndPort = Uv::ToIpPortString(addr);
				auto&& idx = shakes.Find(ipAndPort);			// find by addr
				if (idx == -1 || shakes.ValueAt(idx).first != conv) return 0;	// not found or bad conv: ignore
				shakes.RemoveAt(idx);							// remove from shakes
				peer = xx::TryMake<UvKcpPeerBase>(uv);			// create kcp peer
				if (!peer) return 0;
				peer->udp = SharedFromThis(this);
				peer->conv = conv;
				peer->createMS = NowSteadyEpochMilliseconds();
				memcpy(&peer->addr, addr, sizeof(sockaddr_in6));// upgrade peer's tar addr( maybe accept callback need it )
				if (peer->InitKcp()) return 0;					// init failed: ignore
				peers[conv] = peer;								// store
				owner->Accept(peer);							// accept callback
			}
			else {
				peer = peers.ValueAt(peerIter).Lock();
				if (!peer || peer->Disposed()) return 0;		// disposed: ignore
			}

			memcpy(&peer->addr, addr, sizeof(sockaddr_in6));	// upgrade peer's tar addr
			if (peer->Input(recvBuf, recvLen)) {
				peer->Dispose();								// peer will remove self from peers
			}
			return 0;
		}
	};

	struct UvDialerKcp : UvKcp {
		int i = 0;
		bool connected = false;
		Weak<UvKcpPeerBase> peer_w;
		UvTimer_s timer;
		UvDialerBase* owner = nullptr;			// fill by owner

		UvDialerKcp(Uv& uv, std::string const& ip, int const& port, bool const& isListener)
			: UvKcp(uv, ip, port, isListener) {
			MakeTo(timer, uv, 10, 10, [this] {
				this->Update(NowSteadyEpochMilliseconds());
				});
		}
		~UvDialerKcp() { this->Dispose(-1); }

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!this->UvKcp::Dispose(0)) return false;
			if (auto && peer = peer_w.Lock()) {
				peer->Dispose(flag == -1 ? 0 : 1);
			}
			uv.udps.Remove(port);
			return true;
		}

		inline virtual void Update(int64_t const& nowMS) noexcept {
			if (connected) {
				if (auto && peer = peer_w.Lock()) {
					peer->Update(nowMS);
				}
				else {
					Dispose();
				}
			}
			else {
				++i;
				if ((i & 0xFu) == 0) {		// 每 16 帧发送一次
					if (Send((uint8_t*)& port, sizeof(port))) {
						Dispose();
					}
				}
			}
		}

		inline virtual void Remove(uint32_t const& conv) noexcept override {
			Dispose();
		}

	protected:
		virtual int Unpack(uint8_t* const& recvBuf, uint32_t const& recvLen, sockaddr const* const& addr) noexcept override {
			assert(owner || peer_w.Lock());

			// ensure hand shake result data
			if (recvLen == 8 && owner) {						// 4 bytes serial + 4 bytes conv
				if (memcmp(recvBuf, &port, 4)) return 0;		// bad serial: ignore
				auto&& p = xx::TryMake<UvKcpPeerBase>(uv);
				if (!p) return -1;								// not enough memory
				peer_w = p;										// bind
				auto&& self = As<UvKcp>(SharedFromThis(this));
				p->udp = self;
				memcpy(&p->conv, recvBuf + 4, 4);
				memcpy(&p->addr, addr, sizeof(sockaddr_in6));	// upgrade peer's tar addr
				p->createMS = NowSteadyEpochMilliseconds();
				if (p->InitKcp()) return 0;						// init kcp fail: ignore
				int r = p->Send((uint8_t*)"\x1\0\0\0\0", 5);	// for server accept
				assert(!r);
				(void)r;
				p->Flush();
				connected = true;								// set flag
				owner->Accept(p);								// cleanup all reqs
				owner = nullptr;
				return 0;
			}

			if (recvLen < 24) {									// ignore non kcp data
				return 0;
			}

			auto&& peer = peer_w.Lock();
			if (!peer) return 0;								// hand shake not finish: ignore

			uint32_t conv;
			memcpy(&conv, recvBuf, sizeof(conv));
			if (peer->conv != conv) return 0;					// bad conv: ignore

			memcpy(&peer->addr, addr, sizeof(sockaddr_in6));	// upgrade peer's tar addr
			if (peer->Input(recvBuf, recvLen)) {				// input data to kcp
				peer->Dispose();								// if fail: sucide
			}
			return 0;
		}
	};

	struct UvKcpListener : UvListenerBase {
		Shared<UvListenerKcp> udp;

		UvKcpListener(Uv& uv, std::string const& ip, int const& port)
			: UvListenerBase(uv) {
			auto&& udps = uv.udps;
			auto&& idx = udps.Find(port);
			if (idx != -1) {
				udp = As<UvListenerKcp>(udps.ValueAt(idx).Lock());
				if (udp->owner) throw - 1;						// same port listener already exists?
			}
			else {
				MakeTo(udp, uv, ip, port, true);
				udp->port = port;
				udp->owner = this;
				udps[port] = udp;
			}
			udp->owner = this;
		}
		~UvKcpListener() { this->Dispose(-1); }

		virtual bool Disposed() const noexcept override {
			return !udp;
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!udp) return false;
			udp->owner = nullptr;								// unbind
			udp.Reset();
			if (flag != -1) {
				listener->Dispose(flag);
			}
			return true;
		}
	};

	struct UvTcpListener : UvListenerBase {
		uv_tcp_t* uvTcp = nullptr;
		sockaddr_in6 addr;

		UvTcpListener(Uv& uv, std::string const& ip, int const& port, int const& backlog = 128)
			: UvListenerBase(uv) {
			uvTcp = Uv::Alloc<uv_tcp_t>(this);
			if (!uvTcp) throw - 4;
			if (int r = uv_tcp_init(&uv.uvLoop, uvTcp)) {
				uvTcp = nullptr;
				throw r;
			}

			if (ip.find(':') == std::string::npos) {
				if (uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)& addr)) throw - 1;
			}
			else {
				if (uv_ip6_addr(ip.c_str(), port, &addr)) throw - 2;
			}
			if (uv_tcp_bind(uvTcp, (sockaddr*)& addr, 0)) throw - 3;

			if (uv_listen((uv_stream_t*)uvTcp, backlog, [](uv_stream_t* server, int status) {
				if (status) return;
				auto&& self = Uv::GetSelf<UvTcpListener>(server);
				auto&& peer = xx::TryMake<UvTcpPeerBase>(self->uv);
				if (!peer) return;
				if (uv_accept(server, (uv_stream_t*)peer->uvTcp)) return;
				if (peer->ReadStart()) return;
				Uv::FillIP(peer->uvTcp, peer->ip);
				self->Accept(peer);
				})) throw - 4;
		};

		UvTcpListener(UvTcpListener const&) = delete;
		UvTcpListener& operator=(UvTcpListener const&) = delete;
		~UvTcpListener() { this->Dispose(-1); }

		inline virtual bool Disposed() const noexcept override {
			return !uvTcp;
		}

		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (!uvTcp) return false;
			Uv::HandleCloseAndFree(uvTcp);
			if (flag != -1) {
				listener->Dispose(flag);
			}
			return true;
		}
	};

	inline UvListener::UvListener(Uv& uv, std::string const& ip, int const& port, int const& tcpKcpOpt)
		: UvCreateAcceptBase(uv) {
		if (tcpKcpOpt == 0 || tcpKcpOpt == 2) {
			xx::MakeTo(tcpListener, uv, ip, port);
			tcpListener->listener = this;
		}
		if (tcpKcpOpt == 1 || tcpKcpOpt == 2) {
			xx::MakeTo(kcpListener, uv, ip, port);
			kcpListener->listener = this;
		}
	}

	inline bool UvListener::Dispose(int const& flag) noexcept {
		if (Disposed()) return false;
		tcpListener.Reset();
		kcpListener.Reset();
		auto&& holder = SharedFromThis(this);
		onCreatePeer = nullptr;
		onAccept = nullptr;
		return true;
	}

	struct UvDialer : UvCreateAcceptBase {
		std::function<void(UvPeer_s peer)>& onConnect;	// same as onAccept
		Shared<UvDialerBase> kcpDialer;
		Shared<UvDialerBase> tcpDialer;
		UvTimer_s timeouter;
		bool disposed = false;

		inline bool Busy() {
			return (bool)timeouter;
		}

		UvDialer(Uv& uv, int const& tcpKcpOpt = 2);	// 0: tcp    1: kcp   2: both
		UvDialer(UvDialer const&) = delete;
		UvDialer& operator=(UvDialer const&) = delete;

		~UvDialer() { this->Dispose(-1); }
		virtual bool Disposed() const noexcept override {
			return disposed;
		}
		virtual bool Dispose(int const& flag = 1) noexcept override;
		virtual int Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS = 2000) noexcept;
		virtual int Dial(std::vector<std::string> const& ips, int const& port, uint64_t const& timeoutMS = 2000) noexcept;
		virtual int Dial(std::vector<std::pair<std::string, int>> const& ipports, uint64_t const& timeoutMS = 2000) noexcept;
		virtual void Cancel() noexcept;

		inline int SetTimeout(uint64_t const& timeoutMS = 0) noexcept {
			if (disposed) return -1;
			timeouter.Reset();
			if (!timeoutMS) return 0;
			xx::TryMakeTo(timeouter, uv, timeoutMS, 0, [self_w = SharedFromThis(this).ToWeak()]{
				if (auto && self = self_w.Lock()) {
					self->Cancel();
					self->Accept(UvPeer_s());
				}
				});
			return timeouter ? 0 : -2;
		}
	};

	inline int UvDialerBase::Dial(std::vector<std::string> const& ips, int const& port, uint64_t const& timeoutMS) noexcept {
		if (Disposed()) return -1;
		Cancel();
		for (auto&& ip : ips) {
			if (int r = Dial(ip, port, 0, false)) return r;
		}
		return 0;
	}
	inline int UvDialerBase::Dial(std::vector<std::pair<std::string, int>> const& ipports, uint64_t const& timeoutMS) noexcept {
		if (Disposed()) return -1;
		Cancel();
		for (auto&& ipport : ipports) {
			if (int r = Dial(ipport.first, ipport.second, 0, false)) return r;
		}
		return 0;
	}
	inline void UvDialerBase::Accept(UvPeerBase_s pb) noexcept {
		assert(pb);
		auto&& p = dialer->CreatePeer();
		if (!p) return;
		p->peerBase = pb;
		pb->peer = &*p;
		dialer->Cancel();
		dialer->Accept(p);
	}

	struct UvKcpDialer : UvDialerBase {
		using UvDialerBase::UvDialerBase;
		Dict<int, Shared<UvDialerKcp>> reqs;		// key: port

		inline virtual UvPeer_s CreatePeer() noexcept {
			return dialer->CreatePeer();
		}

		~UvKcpDialer() { this->Dispose(-1); }
		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (disposed) return false;
			disposed = true;
			Cancel();
			if (flag != -1) {
				dialer->Dispose(flag);
			}
			return true;
		}

		inline virtual int Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS = 0, bool cleanup = true) noexcept override {
			if (disposed) return -1;
			if (cleanup) {
				Cancel();
			}
			auto&& req = TryMake<UvDialerKcp>(uv, ip, port, false);
			if (!req) return -2;
			req->owner = this;
			req->port = --uv.autoId;
			uv.udps[req->port] = req;
			reqs[req->port] = std::move(req);
			return 0;
		}

		inline virtual void Cancel() noexcept override {
			if (disposed) return;
			reqs.Clear();
		}
	};

	struct UvTcpDialer;
	struct uv_connect_t_ex {
		uv_connect_t req;
		Shared<UvTcpPeerBase> peer;
		Weak<UvTcpDialer> dialer_w;
		~uv_connect_t_ex();
	};

	struct UvTcpDialer : UvDialerBase {
		using UvDialerBase::UvDialerBase;
		std::vector<uv_connect_t_ex*> reqs;

		~UvTcpDialer() { this->Dispose(-1); }
		inline virtual bool Dispose(int const& flag = 1) noexcept override {
			if (disposed) return false;
			disposed = true;
			Cancel();
			if (flag != -1) {
				dialer->Dispose(flag);
			}
			return true;
		}

		inline virtual int Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS = 0, bool cleanup = true) noexcept override {
			if (disposed) return -1;
			if (cleanup) {
				Cancel();
			}

			sockaddr_in6 addr;
			if (ip.find(':') == std::string::npos) {								// ipv4
				if (int r = uv_ip4_addr(ip.c_str(), port, (sockaddr_in*)& addr)) return r;
			}
			else {																	// ipv6
				if (int r = uv_ip6_addr(ip.c_str(), port, &addr)) return r;
			}

			auto&& req = new (std::nothrow) uv_connect_t_ex();
			if (!req) return -1;
			auto sgReq = xx::MakeScopeGuard([&req] { delete req; });

			req->peer = xx::TryMake<UvTcpPeerBase>(uv);
			if (!req->peer) return -2;

			req->dialer_w = As<UvTcpDialer>(SharedFromThis(this));

			if (uv_tcp_connect(&req->req, req->peer->uvTcp, (sockaddr*)& addr, [](uv_connect_t* conn, int status) {
				Shared<UvTcpDialer> dialer;
				Shared<UvTcpPeerBase> peer;
				{
					// auto delete when exit scope
					auto&& req = std::unique_ptr<uv_connect_t_ex>(container_of(conn, uv_connect_t_ex, req));
					if (status) return;								// error or -4081 canceled
					if (!req->peer) return;							// canceled
					dialer = req->dialer_w.Lock();
					if (!dialer) return;							// container disposed
					peer = std::move(req->peer);					// remove peer to outside, avoid cancel
				}
				if (peer->ReadStart()) return;						// read error
				Uv::FillIP(peer->uvTcp, peer->ip);
				dialer->Accept(peer);								// callback
				})) return -3;

			reqs.push_back(req);
			sgReq.Cancel();
			return 0;
		}

		inline virtual void Cancel() noexcept override {
			if (disposed) return;
			if (!reqs.empty()) {
				for (auto i = reqs.size() - 1; i != (size_t)-1; --i) {
					auto req = reqs[i];
					assert(req->peer);
					uv_cancel((uv_req_t*)& req->req);				// ios call this do nothing
					if (reqs[i] == req) {							// check req is alive
						req->peer.Reset();							// ios need this to fire cancel progress
					}
				}
			}
			reqs.clear();
		}
	};

	inline uv_connect_t_ex::~uv_connect_t_ex() {
		auto&& dialer = dialer_w.Lock();
		if (!dialer) return;
		for (auto& p : dialer->reqs) {
			if (p == this) {
				p = dialer->reqs.back();
				dialer->reqs.pop_back();
				return;
			}
		}
	}

	inline UvDialer::UvDialer(Uv& uv, int const& tcpKcpOpt)
		: UvCreateAcceptBase(uv)
		, onConnect(this->onAccept) {
		if (tcpKcpOpt < 0 || tcpKcpOpt>2) throw - 1;
		if (tcpKcpOpt == 0 || tcpKcpOpt == 2) {
			tcpDialer = xx::Make<UvTcpDialer>(uv);
			tcpDialer->dialer = this;
		}
		if (tcpKcpOpt == 1 || tcpKcpOpt == 2) {
			kcpDialer = xx::Make<UvKcpDialer>(uv);
			kcpDialer->dialer = this;
		}
	}
	inline int UvDialer::Dial(std::string const& ip, int const& port, uint64_t const& timeoutMS) noexcept {
		if (int r = SetTimeout(timeoutMS)) return r;
		if (tcpDialer) {
			if (int r = tcpDialer->Dial(ip, port, timeoutMS)) return r;
		}
		if (kcpDialer) {
			if (int r = kcpDialer->Dial(ip, port, timeoutMS)) return r;
		}
		return 0;
	}
	inline int UvDialer::Dial(std::vector<std::string> const& ips, int const& port, uint64_t const& timeoutMS) noexcept {
		if (int r = SetTimeout(timeoutMS)) return r;
		if (tcpDialer) {
			if (int r = tcpDialer->Dial(ips, port, timeoutMS)) return r;
		}
		if (kcpDialer) {
			if (int r = kcpDialer->Dial(ips, port, timeoutMS)) return r;
		}
		return 0;
	}
	inline int UvDialer::Dial(std::vector<std::pair<std::string, int>> const& ipports, uint64_t const& timeoutMS) noexcept {
		if (int r = SetTimeout(timeoutMS)) return r;
		if (tcpDialer) {
			if (int r = tcpDialer->Dial(ipports, timeoutMS)) return r;
		}
		if (kcpDialer) {
			if (int r = kcpDialer->Dial(ipports, timeoutMS)) return r;
		}
		return 0;
	}
	inline void UvDialer::Cancel() noexcept {
		timeouter.Reset();
		if (tcpDialer) {
			tcpDialer->Cancel();
		}
		if (kcpDialer) {
			kcpDialer->Cancel();
		}
	}

	inline bool UvDialer::Dispose(int const& flag) noexcept {
		if (disposed) return false;
		disposed = true;
		Cancel();
		if (flag != -1) {
			if (tcpDialer) {
				tcpDialer->Dispose(flag);
			}
			if (kcpDialer) {
				kcpDialer->Dispose(flag);
			}
		}
		return true;
	}

	using UvDialer_s = Shared<UvDialer>;
	using UvDialer_w = Weak<UvDialer>;
}
