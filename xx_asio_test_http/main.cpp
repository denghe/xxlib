#include <xx_data_shared.h>
#include <xx_string.h>
#include <xx_ptr.h>
#include <picohttpparser.h>
#include <xxh3.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
using asio::co_spawn;
using asio::awaitable;
using asio::detached;


// 协程内使用的 通用超时函数, 实现 sleep 的效果
inline awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}


// 可用于 服务主体 或 worker 继承使用，以方便各处传递调用
struct IOCBase : asio::io_context {
	using asio::io_context::io_context;

	// 开始监听一个 tcp 端口
	template<typename SocketHandler>
	void Listen(uint16_t const& port, SocketHandler&& sh) {
		co_spawn(*this, [this, port, sh = std::move(sh)]()->awaitable<void> {
			asio::ip::tcp::acceptor acceptor(*this, { asio::ip::tcp::v6(), port });
			acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			for (;;) {
				asio::ip::tcp::socket socket(*this);
				if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
					sh(std::move(socket));
				}
				else break;
			}
		}, detached);
	}
};


template<size_t headersCap = 32>
struct HttpParser {
	std::string_view method;
	std::string_view path;
	std::array<std::pair<std::string_view, std::string_view>, headersCap> headers;
	size_t numHeaders;
	int minorVersion;

	struct Filler {
		const char* method;
		size_t methodLen;
		const char* path;
		size_t pathLen;
		std::array<phr_header, headersCap> headers;
	};

	// -1: failed   -2: partial   s.size(): successful
	int Parse(std::string_view const& s, size_t lastLen = 0) {
		numHeaders = headersCap;
		auto& z = *(Filler*)this;
		return phr_parse_request(s.data(), s.size()
			, &z.method, &z.methodLen
			, &z.path, &z.pathLen
			, &minorVersion
			, z.headers.data(), &numHeaders
			, lastLen);
	}
};


#define PEERTHIS ((PeerDeriveType*)(this))

// 最终 Peer 应使用 xx::Shared 包裹使用，以方便 co_spawn 捕获加持, 确保生命周期长于协程
template<class PeerDeriveType, typename ServerType, size_t readBufLen = 4194304, size_t headersCap = 32>
struct PeerBaseCode : asio::noncopyable {
	ServerType& server;
	asio::ip::tcp::socket socket;
	asio::steady_timer writeBlocker;
	std::deque<xx::DataShared> writeQueue;
	bool stoped = true;
	HttpParser<headersCap> parser;

	PeerBaseCode(ServerType& server_) : server(server_), socket(server_), writeBlocker(server_) {}
	PeerBaseCode(ServerType& server_, asio::ip::tcp::socket&& socket_) : server(server_), socket(std::move(socket_)), writeBlocker(server_) {}

	// 初始化。通常于 peer 创建后立即调用
	void Start() {
		if (!stoped) return;
		stoped = false;
		writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
		writeQueue.clear();
		co_spawn(server, [self = xx::SharedFromThis(this)] { return self->Read(); }, detached);
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


	inline static constexpr std::string_view prefixText =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain;charset=utf-8\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: "sv;

	inline static constexpr std::string_view prefixHtml =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html;charset=utf-8\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: "sv;

	inline static constexpr std::string_view prefix404 =
		"HTTP/1.1 404 Not Found\r\n"
		"Content-Type: text/html;charset=utf-8\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: "sv;

	// 临时容器
	std::string lenStr;

	// 用来返回 http 内容
	int SendResponse(std::string_view const& prefix, std::string_view const& content) {
		xx::Data d(/* partial http header */prefix.size() + /* len */20 + /*\r\n\r\n*/4 + /* body */content.size() + /* refs */8);
		d.WriteBuf(prefix.data(), prefix.size());
		lenStr.clear();
		xx::Append(lenStr, content.size());
		d.WriteBuf(lenStr.data(), lenStr.size());
		d.WriteBuf("\r\n\r\n", 4);
		d.WriteBuf(content.data(), content.size());
		Send(std::move(d));
		return 0;
	};

protected:
	// 读 协程函数主体
	awaitable<void> Read() {
		auto buf_ = std::make_unique<char[]>(readBufLen);

		auto buf = buf_.get();
		size_t buflen = 0, prevbuflen = 0;
		int rtv = 0;

		for (;;) {
			auto [ec, recvlen] = co_await socket.async_read_some(asio::buffer(buf + buflen, readBufLen - buflen), use_nothrow_awaitable);
			if (ec) break;
			if (stoped) co_return;
			prevbuflen = buflen;
			buflen += recvlen;

			rtv = parser.Parse(std::string_view(buf, buflen), prevbuflen);

			if (rtv > 0) {
				// success
				if (int r = PEERTHIS->ReceiveHttpRequest()) {
					// disconnect?
					break;
				}
				continue;
			}
			else if (rtv == -1) {
				// parse error
				break;
			}
			assert(rtv == -2);

			if (buflen == readBufLen) {
				// too long
				break;
			}
		}
		Stop();
	}

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct HttpServer;

struct HttpPeer : PeerBaseCode<HttpPeer, HttpServer> {
	using PeerBaseCode::PeerBaseCode;
	int ReceiveHttpRequest();
};

struct SVHasher {
	using hash_type = xx::Hash<std::string_view>; 
	using is_transparent = void;
	size_t operator()(const char* str) const { return hash_type{}(str); }
	size_t operator()(std::string_view str) const { return hash_type{}(str); }
	size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

struct HttpServer : IOCBase {
	using IOCBase::IOCBase;
	std::unordered_map<std::string, std::function<int(HttpPeer&)>, SVHasher, std::equal_to<void>> handlers;
};

inline int HttpPeer::ReceiveHttpRequest() {
	// todo: split path by / ?
	auto iter = server.handlers.find(parser.path);
	if (iter == server.handlers.end()) {
		std::cout << "unhandled path: " << parser.path << std::endl;
		return -1;
	}
	return iter->second(*this);
}

int main() {
	HttpServer hs(1);

	hs.Listen(12345, [&](auto&& socket) {
		auto p = xx::Make<HttpPeer>(hs, std::move(socket));
		p->Start();
	});
	std::cout << "HttpListen 12345" << std::endl;

	hs.handlers["/"] = [](HttpPeer& p)->int {
		auto& parser = p.parser;
		std::cout << "method: " << parser.method << std::endl;
		std::cout << "path: " << parser.path << std::endl;
		std::cout << "minorVersion: " << parser.minorVersion << std::endl;
		std::cout << "numHeaders: " << parser.numHeaders << std::endl;
		for (size_t i = 0; i < parser.numHeaders; ++i) {
			std::cout << "kv: " << parser.headers[i].first << ":" << parser.headers[i].second << std::endl;
		}
		std::cout << std::endl;

		p.SendResponse(p.prefix404, "<html><body>home!!!</body></html>");
		return 0;
	};

	hs.run();
	return 0;
}
