#include <xx_asio_tcp_base_http.h>

// 适配 std::string[_view] 走 xxhash 以提速
#include <xxh3.h>
namespace xx {
	template<typename T>
	struct Hash<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>
		|| std::is_same_v<std::decay_t<T>, std::string_view>>> {
		inline size_t operator()(T const& k) const {
			return (size_t)XXH3_64bits(k.data(), k.size());
		}
	};
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

struct IOC;
struct ServerHttpPeer : xx::PeerTcpBaseCode<ServerHttpPeer, IOC, 100> , xx::PeerHttpCode<ServerHttpPeer, 1024 * 32> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	// path 对应的 处理函数 容器( 扫 array 比 扫 map 快得多的多 )
	inline static std::array<std::pair<std::string_view, std::function<int(ServerHttpPeer&)>>, 32> handlers;
	inline static size_t handlersLen;

	// 处理收到的 http 请求( 会被 PeerHttpCode 基类调用 )
	int ReceiveHttpRequest();
};

// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
};

inline int ServerHttpPeer::ReceiveHttpRequest() {
	std::cout << GetDumpStr() << std::endl;		// 打印一下收到的东西

	// 对 url 进一步解析, 切分出 path 和 args
	FillPathAndArgs();

	// 在 path 对应的 处理函数 容器 中 定位并调用
	for (size_t i = 0; i < ServerHttpPeer::handlersLen; i++) {
		if (path == ServerHttpPeer::handlers[i].first) {
			return ServerHttpPeer::handlers[i].second(*this);
		}
	}
	SendResponse(prefix404, "<html><body>404 !!!</body></html>"sv);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

struct ClientHttpPeer : xx::PeerTcpBaseCode<ClientHttpPeer, IOC, 100> , xx::PeerHttpCode<ClientHttpPeer, 1024 * 32> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	// 回包 对应的 处理函数 容器
	inline static std::unordered_map<std::string, std::function<int(ServerHttpPeer&)>
		, xx::StringHasher<>, std::equal_to<void>> httpResponseHandlers;

	void Start_() {
		// todo: 起协程开始测试
	}

	// 处理收到的 http 请求
	int ReceiveHttpRequest() {
		// todo
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
	IOC ioc(1);

	// 监听 http
	ioc.Listen(12345, [&](auto&& socket) {
		xx::Make<ServerHttpPeer>(ioc, std::move(socket))->Start();
		});
	std::cout << "***** http port: 12345" << std::endl;

	// 注册 http 处理函数
	ServerHttpPeer::handlers[ServerHttpPeer::handlersLen++] = std::make_pair("/"sv, [](ServerHttpPeer& p)->int {
		p.SendResponse(p.prefix404, "<html><body>home!!!</body></html>");
		return 0;
	});

	// 起一个协程来实现 创建一份 client 并 自动连接到 server
	co_spawn(ioc, [&]()->awaitable<void> {
		// 创建 client peer
		auto cp = xx::Make<ClientHttpPeer>(ioc);

		// 反复测试，不退出
		while (!ioc.stopped()) {
			// 如果已断开 就尝试重连
			if (cp->IsStoped()) {
				std::cout << "***** connecting..." << std::endl;
				if (0 == co_await cp->Connect(asio::ip::address::from_string("127.0.0.1"), 12345)) {
					std::cout << "***** connected" << std::endl;
				}
			}
			// 频率控制在 1 秒 2 次
			co_await xx::Timeout(500ms);
		}
		}, detached);

	ioc.run();
	return 0;
}
