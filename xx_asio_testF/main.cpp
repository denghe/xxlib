#pragma region includes

#ifdef USE_STANDALONE_ASIO
#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#else
#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
#endif

using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
using asio::co_spawn;
using asio::awaitable;
using asio::detached;

#include <deque>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
using namespace std::chrono_literals;

#pragma endregion

inline awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

struct Listener : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;
	Listener() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

	template<typename SocketHandler>
	void Listen(uint16_t const& port, SocketHandler&& sh) {
		co_spawn(ioc, [this, port, sh = std::move(sh)]()->awaitable<void> {
			asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });
			acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			for (;;) {
				asio::ip::tcp::socket socket(ioc);
				if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
					//socket.set_option(asio::ip::tcp::no_delay(true));
					sh(std::move(socket));
				}
			}
		}, detached);
	}
};

/*************************************************************************************************************************/
/*************************************************************************************************************************/
/*************************************************************************************************************************/

struct Data_r {
	uint8_t* buf = nullptr;
	size_t len = 0;
	size_t offset = 0;

	template<typename T>
	int ReadFixed(T& v) {
		if (offset + sizeof(T) > len) return __LINE__;
		memcpy(&v, buf + offset, sizeof(T));
		offset += sizeof(T);
		return 0;
	}

	template<typename T>
	int ReadFixedAt(size_t const& idx, T& v) {
		if (idx + sizeof(T) > len) return __LINE__;
		memcpy(&v, buf + idx, sizeof(T));
		return 0;
	}
};

struct Data : Data_r {
	size_t cap = 0;

	Data(Data&& o) noexcept {
		memcpy((void*)this, &o, sizeof(Data));
		memset((void*)&o, 0, sizeof(Data));
	}
	Data& operator=(Data&& o) noexcept {
		std::swap(buf, o.buf);
		std::swap(len, o.len);
		std::swap(cap, o.cap);
		std::swap(offset, o.offset);
		return *this;
	}
	Data(Data&) = delete;
	Data& operator=(Data&) = delete;
	Data() = default;
	Data(void* buf_, size_t len_, size_t offset_ = 0) {
		Resize(len_);
		memcpy(buf, buf_, len_);
		offset = offset_;
	}
	~Data() {
		Clear(true);
	}

	void Reserve(size_t const& newCap) {
		if (newCap <= cap) return;
		if (cap) {
			do {
				cap *= 2;
			} while (cap < newCap);
			auto newBuf = ((uint8_t*)malloc(cap));
			if (len) {
				memcpy(newBuf, buf, len);
			}
			free(buf);
			buf = newBuf;
		}
		else {
			buf = ((uint8_t*)malloc(newCap));
			cap = newCap;
		}
	}

	void Resize(size_t const& newLen) {
		Reserve(newLen);
		len = newLen;
	}

	template<bool checkReserve = true, typename T>
	void WriteFixed(T&& v) {
		if constexpr (checkReserve) {
			Reserve(len + sizeof(T));
		}
		memcpy(buf + len, &v, sizeof(T));
		len += sizeof(T);
	}

	template<bool checkReserve = true, typename T>
	void WriteFixedAt(size_t const& idx, T v) {
		if constexpr (checkReserve) {
			if (idx + sizeof(T) > len) {
				Resize(sizeof(T) + idx);
			}
		}
		memcpy(buf + idx, &v, sizeof(T));
	}

	// { 1,2,3. ....}
	template<typename T = int32_t, typename = std::enable_if_t<std::is_convertible_v<T, uint8_t>>>
	void Fill(std::initializer_list<T> const& bytes) {
		Clear();
		Reserve(bytes.size());
		for (auto&& b : bytes) {
			buf[len++] = (uint8_t)b;
		}
	}

	void Clear(bool const& freeBuf = false) {
		if (freeBuf && cap) {
			free(buf);
			buf = nullptr;
			cap = 0;
		}
		len = 0;
		offset = 0;
	}

	void RemoveFront(size_t const& siz) {
		assert(siz <= len);
		if (!siz) return;
		len -= siz;
		if (len) {
			memmove(buf, buf + siz, len);
		}
	}
};


/*************************************************************************************************************************/
/*************************************************************************************************************************/
/*************************************************************************************************************************/

//#define ENABLE_READ_HEADER_READ_DATA_MODE


#define PEERTHIS ((PeerDeriveType*)(this))

template<class PeerDeriveType>
struct PeerBaseCode {
	asio::io_context& ioc;
	asio::ip::tcp::socket socket;
	asio::steady_timer writeBlocker;
	std::deque<Data> writeQueue;
	bool stoped = true;
	PeerBaseCode(asio::io_context& ioc_) : ioc(ioc_), socket(ioc_), writeBlocker(ioc_) {}
	PeerBaseCode(asio::io_context& ioc_, asio::ip::tcp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), writeBlocker(ioc_) {}

	void Start() {
		if (!stoped) return;
		stoped = false;
		writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
		writeQueue.clear();
		co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Read(); }, detached);
		co_spawn(ioc, [self = PEERTHIS->shared_from_this()]{ return self->Write(); }, detached);
	}

	void Stop() {
		if (stoped) return;
		stoped = true;
		socket.close();
		writeBlocker.cancel();
	}

	void Send(Data&& buf) {
		if (stoped) return;
		writeQueue.emplace_back(std::move(buf));
		writeBlocker.cancel_one();
	}

	// for connect to listener
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
#ifdef ENABLE_READ_HEADER_READ_DATA_MODE
	awaitable<void> Read() {
		for (;;) {
			Data d;
			{
				// 读 2 字节 包头( 含自身长度 )
				uint16_t dataLen = 0;
				{
					auto [ec, n] = co_await asio::async_read(socket, asio::buffer(&dataLen, 1), use_nothrow_awaitable);
					if (ec) break;
				}
				{
					auto [ec, n] = co_await asio::async_read(socket, asio::buffer((char*)&dataLen + 1, 1), use_nothrow_awaitable);
					if (ec) break;
				}
				if (!dataLen) break;
				if (stoped) co_return;
				d.Resize(dataLen);
				d.WriteFixedAt(0, dataLen);		// 顺便也把 包头 写入容器 以方便某些转发逻辑
			}
			{
				// 读 数据
				auto [ec, n] = co_await asio::async_read(socket, asio::buffer(d.buf + sizeof(uint16_t), d.len - sizeof(uint16_t)), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
			}
			if (auto r = PEERTHIS->HandleMessage(std::move(d))) {							// PeerDeriveType need supply this function: int HandleMessage(Data&& d)
				Stop();
			}
			if (stoped) co_return;
		}
		Stop();
	}
#else
	// 另外一种写法. 少一次 read, 数据引用处理
	awaitable<void> Read() {
		uint8_t buf[16384];
		size_t len = 0;
		uint16_t dataLen;
		Data_r dr;
		for (;;) {
			auto [ec, n] = co_await socket.async_read_some(asio::buffer(buf + len, sizeof(buf) - len), use_nothrow_awaitable);
			if (ec) break;
			if (stoped) co_return;
			len += n;

			auto p = buf;
			auto pe = buf + len;
		LabBegin:
			if (p + sizeof(uint16_t) > pe) goto LabEnd;
			// 读 2 字节 包头( 含自身长度 )
			dataLen = (uint16_t)( p[0] | (p[1] << 8) );
			if (p + dataLen > pe) goto LabEnd;
			dr.buf = p;
			dr.len = dataLen;
			dr.offset = 0;
			if (int r = PEERTHIS->HandleMessage(dr)) break;
			if (stoped) co_return;
			p += dataLen;
			goto LabBegin;
		LabEnd:
			if (p != buf) {
				if ((len = pe - p)) {
					memmove(buf, p, len);
				}
			}
		}
		Stop();
	}
#endif


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

/*************************************************************************************************************************/
/*************************************************************************************************************************/
/*************************************************************************************************************************/

template<typename PeerDeriveType>
struct PeerReqCode : PeerBaseCode<PeerDeriveType> {
	using PBC = PeerBaseCode<PeerDeriveType>;
	using PBC::PBC;
	int32_t reqAutoId = 0;
	std::unordered_map<int32_t, std::pair<asio::steady_timer, Data>> reqs;

	template<size_t reserveLen = 128, typename F>
	void SendResponse(int32_t const& serial, F&& dataFiller) {
		if (PEERTHIS->stoped) return;
		Data d;
		d.Reserve(reserveLen);
		d.len = sizeof(uint16_t) + sizeof(int32_t);
		dataFiller(d);
		if (d.len > std::numeric_limits<uint16_t>::max()) return;	// break? log?
		d.WriteFixedAt<false>(0, (uint16_t)d.len);
		d.WriteFixedAt<false>(2, serial);
		PEERTHIS->Send((std::move(d)));
	}

	template<size_t reserveLen = 128, typename F>
	void SendPush(F&& dataFiller) {
		SendResponse<reserveLen>(0, std::forward<F>(dataFiller));
	}

	template<size_t reserveLen = 128, typename F>
	awaitable<Data> SendRequest(std::chrono::steady_clock::duration d, F&& dataFiller) {
		if (PEERTHIS->stoped) co_return Data();
		reqAutoId = (reqAutoId + 1) % 0x7FFFFFFF;
		auto key = reqAutoId;
		auto result = reqs.emplace(reqAutoId, std::make_pair(asio::steady_timer(PEERTHIS->ioc, std::chrono::steady_clock::now() + d), Data()));
		assert(result.second);
		SendResponse<reserveLen>(-reqAutoId, std::forward<F>(dataFiller));
		co_await result.first->second.first.async_wait(use_nothrow_awaitable);
		if (PEERTHIS->stoped) co_return Data();
		auto r = std::move(result.first->second.second);
		reqs.erase(result.first);
		co_return r;
	}

#ifdef ENABLE_READ_HEADER_READ_DATA_MODE
	int HandleMessage(Data&& d) {
		int32_t serial;
		if (d.ReadFixedAt(2, serial)) return __LINE__;
		d.offset = sizeof(uint16_t) + sizeof(int32_t);	// skip header area

		if (serial > 0) {
			if (auto iter = reqs.find(serial); iter != reqs.end()) {
				iter->second.second = std::move(d);
				iter->second.first.cancel();
			}
		}
		else {
			if (serial == 0) {
				if (PEERTHIS->ReceivePush(std::move(d))) return __LINE__;					// PeerDeriveType need supply this function: int ReceivePush(Data&& d)
			}
			else {
				if (PEERTHIS->ReceiveRequest(-serial, std::move(d))) return __LINE__;		// PeerDeriveType need supply this function: int ReceiveRequest(int32_t serial, Data&& d)
			}
		}
		return 0;
	}
#else
	int HandleMessage(Data_r& d) {
		int32_t serial;
		if (d.ReadFixedAt(2, serial)) return __LINE__;
		d.offset = sizeof(uint16_t) + sizeof(int32_t);	// skip header area

		if (serial > 0) {
			if (auto iter = reqs.find(serial); iter != reqs.end()) {
				iter->second.second = Data(d.buf, d.len, d.offset);				// copy to new Data
				iter->second.first.cancel();
			}
		} else {
			if (serial == 0) {
				if (PEERTHIS->ReceivePush(d)) return __LINE__;					// PeerDeriveType need supply this function: int ReceivePush(Data_r& d)
			} else {
				if (PEERTHIS->ReceiveRequest(-serial, d)) return __LINE__;		// PeerDeriveType need supply this function: int ReceiveRequest(int32_t serial, Data_r& d)
			}
		}
		return 0;
	}
#endif
};

/*************************************************************************************************************************/
/*************************************************************************************************************************/
/*************************************************************************************************************************/

#include <iostream>
#include <chrono>

struct Peer : PeerReqCode<Peer>, std::enable_shared_from_this<Peer> {
	using PeerReqCode::PeerReqCode;

	// 模拟 RPC ?
	awaitable<int> Add(int a, int b) {
		uint16_t typeId = 1;
		auto d = co_await SendRequest<32>(3s, [&](Data& d) {
			d.WriteFixed<false>(typeId);
			d.WriteFixed<false>(a);
			d.WriteFixed<false>(b);
		});
		if (!d.len) 
			throw std::logic_error("int Add(a,b) receive 0 len data");
		int rtv;
		if (int r = d.ReadFixed(rtv)) 
			throw std::logic_error("int Add(a,b) receive data read result error");
		co_return rtv;
	}

#ifdef ENABLE_READ_HEADER_READ_DATA_MODE
	int ReceivePush(Data&& d) {
#else
	int ReceivePush(Data_r& d) {
#endif
		return 0;
	}

#ifdef ENABLE_READ_HEADER_READ_DATA_MODE
	int ReceiveRequest(int32_t serial, Data&& d) {
#else
	int ReceiveRequest(int32_t serial, Data_r& d) {
#endif
		uint16_t typeId = 0;
		if (int r = d.ReadFixed(typeId)) return __LINE__;
		switch (typeId) {
		case 1: {
			int a;
			if (int r = d.ReadFixed(a)) return __LINE__;
			int b;
			if (int r = d.ReadFixed(b)) return __LINE__;
			SendResponse<16>(serial, [a, b](Data& d) {
				d.WriteFixed<false>(a + b);
			});
		}
		}
		return 0;
	}
};

struct Server : Listener {
	std::unordered_set<std::shared_ptr<Peer>> ps;
};


int main() {
	Server s;
	s.Listen(12345, [&](asio::ip::tcp::socket&& socket) {
		auto p = std::make_shared<Peer>(s.ioc, std::move(socket));
		s.ps.insert(p);
		p->Start();
	});

	co_spawn(s.ioc, [&]()->awaitable<void> {
		auto p = std::make_shared<Peer>(s.ioc);
		while (p->stoped) {
			co_await p->Connect(asio::ip::address::from_string("127.0.0.1"), 12345);
		}
		std::cout << "connected" << std::endl;
		try {
			auto tp1 = std::chrono::steady_clock::now();
			int n = 0;
			for (size_t i = 0; i < 100000; i++) {
				n = co_await p->Add(n, 1);
			}
			std::cout << "n = " << n << std::endl;
			auto tp2 = std::chrono::steady_clock::now();
			std::cout << "ms = " << std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count() << std::endl;
		}
		catch (std::exception const& e) {
			std::cout << "e = " << e.what() << std::endl;
		}
		p->Stop();
		co_return;
	}, detached);

	s.ioc.run();
	return 0;
}
