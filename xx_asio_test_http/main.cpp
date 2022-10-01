#include <xx_asio_tcp_base_http.h>

#include <xxh3.h>
namespace xx {
	// 适配 std::string[_view] 走 xxhash
	template<typename T>
	struct Hash<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view>>> {
		inline size_t operator()(T const& k) const {
			return (size_t)XXH3_64bits(k.data(), k.size());
		}
	};
}

struct Server;
struct HttpPeer : xx::PeerTcpBaseCode<HttpPeer, Server>, xx::PeerHttpCode<HttpPeer> {
	using PeerTcpBaseCode::PeerTcpBaseCode;
	int ReceiveHttpRequest();
};

struct Server : xx::IOCBase {
	using IOCBase::IOCBase;
	std::unordered_map<std::string, std::function<int(HttpPeer&)>, xx::StringHasher<>, std::equal_to<void>> handlers;
};

inline int HttpPeer::ReceiveHttpRequest() {
	// todo: split path by / ?
	auto iter = server.handlers.find(path);
	if (iter == server.handlers.end()) {
		std::cout << "unhandled path: " << path << std::endl;
		return -1;
	}
	return iter->second(*this);
}

// 走正常 Send 流程 echo ( 会有协程调度延迟 )
struct TcpEchoPeer : xx::PeerTcpBaseCode<TcpEchoPeer, Server> {
	using PeerTcpBaseCode::PeerTcpBaseCode;
	awaitable<void> Read() {
		constexpr size_t blockSiz = 1024 * 1024 * 4;
		auto block = std::make_unique<char[]>(blockSiz);
		for (;;) {
			auto [ec, recvLen] = co_await socket.async_read_some(asio::buffer(block.get(), blockSiz), use_nothrow_awaitable);
			if (ec) break;
			if (this->stoped) co_return;
			xx::Data d;
			d.WriteBuf(block.get(), recvLen);
			Send(std::move(d));
		}
		Stop();
	}
};

#define PING_PONG_TEST 1

// for performance test
struct HttpClientPeer : xx::PeerTcpBaseCode<HttpClientPeer, Server> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	// http request content
	inline static constexpr std::string_view reqStr =
		"GET / HTTP/1.1\r\n"                                                \
		"Host: www.kittyhell.com\r\n"                                                                                                  \
		"User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
		"Pathtraq/0.9\r\n"                                                                                                             \
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
		"Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
		"Accept-Encoding: gzip,deflate\r\n"                                                                                            \
		"Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
		"Keep-Alive: 115\r\n"                                                                                                          \
		"Connection: keep-alive\r\n"                                                                                                   \
		"Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
		"__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
		"__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
		"\r\n"sv;

	// package cache for send
	xx::DataShared reqPkg;

	void Start_() {
		// fill cache package
		xx::Data d;
		d.WriteBuf(reqStr.data(), reqStr.size());
		d.WriteBuf(reqStr.data(), reqStr.size());
		d.WriteBuf(reqStr.data(), reqStr.size());
		reqPkg = xx::DataShared(std::move(d));

#ifdef PING_PONG_TEST
		Send(reqPkg);
#else
		co_spawn(server, [self = xx::SharedFromThis(this)]()->awaitable<void> {
			while (!self->stoped) {
				if (self->writeQueue.size() < 100000) {
					for (size_t i = 0; i < 1; i++) {
                        self->Send(self->reqPkg);
					}
				}
				co_await xx::Timeout(0ms);
			}
		}, detached);
#endif
	}

	size_t count = 0;
	awaitable<void> Read() {
		constexpr size_t blockSiz = 1024 * 1024 * 4;
		auto block = std::make_unique<char[]>(blockSiz);
		for (;;) {
			auto [ec, recvLen] = co_await socket.async_read_some(asio::buffer(block.get(), blockSiz), use_nothrow_awaitable);
			if (ec) break;
			if (this->stoped) co_return;
			count += recvLen;			// 当前返回内容应该是 142 字节, 次数就 除以 142 来算出

#ifdef PING_PONG_TEST
			Send(reqPkg);
#endif
		}
		Stop();
	}
};



int RunServer() {
	Server server(1);

	// 监听 echo
	server.Listen(12333, [&](auto&& socket) {
		xx::Make<TcpEchoPeer>(server, std::move(socket))->Start();
	});
	std::cout << "tcp echo port: 12333" << std::endl;

	// 监听 http
	server.Listen(12345, [&](auto&& socket) {
		xx::Make<HttpPeer>(server, std::move(socket))->Start();
	});
	std::cout << "http manage port: 12345" << std::endl;

	// 注册 http 处理回调
	server.handlers["/"] = [](HttpPeer& p)->int {
		//std::cout << "method: " << p.method << std::endl;
		//std::cout << "path: " << p.path << std::endl;
		//std::cout << "minorVersion: " << p.minorVersion << std::endl;
		//std::cout << "numHeaders: " << p.numHeaders << std::endl;
		//for (size_t i = 0; i < p.numHeaders; ++i) {
		//	std::cout << "kv: " << p.headers[i].first << ":" << p.headers[i].second << std::endl;
		//}
		//std::cout << std::endl;
		p.SendResponse(p.prefix404, "<html><body>home!!!</body></html>");
		return 0;
	};

	server.run();
	return 0;
}

int RunClient() {
	Server server(1);

	// 启动个协程压测一下 http 输出性能
	co_spawn(server, [&]()->awaitable<void> {
		auto cp = xx::Make<HttpClientPeer>(server);
		while (true) {
			if (cp->stoped) {
				co_await xx::Timeout(500ms);
				co_await cp->Connect(asio::ip::address::from_string("127.0.0.1"), 12345);
			}
			while (!cp->stoped) {
				cp->count = 0;
				co_await xx::Timeout(1000ms);
				//std::cout << cp->count << std::endl;
				std::cout << (cp->count / 142) << std::endl;
			}
		}
	}, detached);

	server.run();
	return 0;
}

int main() {
	std::thread t([] {
		RunServer();
	});
	t.detach();
	return RunClient();
    //return RunServer();
}
