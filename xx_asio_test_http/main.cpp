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
	std::unordered_map<std::string, std::function<int(HttpPeer&)>, xx::StringHasher, std::equal_to<void>> handlers;
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
		char buf[1024];
		for (;;) {
			auto [ec, recvLen] = co_await socket.async_read_some(asio::buffer(buf, 1024), use_nothrow_awaitable);
			if (ec) break;
			if (this->stoped) co_return;
			xx::Data d;
			d.WriteBuf(buf, recvLen);
			Send(std::move(d));
		}
		Stop();
	}
};

int main() {
	Server server(1);
	server.Listen(12333, [&](auto&& socket) {
		xx::Make<TcpEchoPeer>(server, std::move(socket))->Start();
	});
	std::cout << "tcp echo port: 12333" << std::endl;

	server.Listen(12345, [&](auto&& socket) {
		xx::Make<HttpPeer>(server, std::move(socket))->Start();
	});
	std::cout << "http manage port: 12345" << std::endl;

	server.handlers["/"] = [](HttpPeer& p)->int {
		std::cout << "method: " << p.method << std::endl;
		std::cout << "path: " << p.path << std::endl;
		std::cout << "minorVersion: " << p.minorVersion << std::endl;
		std::cout << "numHeaders: " << p.numHeaders << std::endl;
		for (size_t i = 0; i < p.numHeaders; ++i) {
			std::cout << "kv: " << p.headers[i].first << ":" << p.headers[i].second << std::endl;
		}
		std::cout << std::endl;
		p.SendResponse(p.prefix404, "<html><body>home!!!</body></html>");
		return 0;
	};

	server.run();
	return 0;
}
