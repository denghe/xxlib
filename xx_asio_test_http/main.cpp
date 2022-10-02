#include <xx_asio_tcp_base_http.h>

// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
	ssize_t count = 0;
};

// serer http peer
struct SHPeer : xx::PeerTcpBaseCode<SHPeer, IOC> , xx::PeerHttpCode<SHPeer> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	// path 对应的 处理函数 容器( 扫 array 比 扫 map 快得多的多 )
	inline static std::array<std::pair<std::string_view, std::function<int(SHPeer&)>>, 32> handlers;
	inline static size_t handlersLen;

	void Start_() {
		std::cout << "***** client connected" << std::endl;
	}

	void Stop_() {
		std::cout << "***** client disconnected" << std::endl;
	}

	// 处理收到的 http 请求( 会被 PeerHttpCode 基类调用 )
	inline int ReceiveHttp() {
		// std::cout << GetDumpStr() << std::endl;					// 打印一下收到的东西
		FillPathAndArgs();											// 对 url 进一步解析, 切分出 path 和 args
		for (size_t i = 0; i < SHPeer::handlersLen; i++) {			// 在 path 对应的 处理函数 容器 中 定位并调用
			if (path == SHPeer::handlers[i].first) {
				return SHPeer::handlers[i].second(*this);
			}
		}
		SendHtml(prefix404, "<html><body>404 !!!</body></html>"sv);	// 返回 资源不存在 的说明
		return 0;
	}
};

int main() {
	IOC ioc(1);

	// 开始监听 http tcp 端口
	ioc.Listen(12345, [&](auto&& socket) {
		xx::Make<SHPeer>(ioc, std::move(socket))->Start();
	});
	std::cout << "***** http port: 12345" << std::endl;

	// 开始注册 http 处理函数
	SHPeer::handlers[SHPeer::handlersLen++] = std::make_pair("/"sv, [&](SHPeer& p)->int {
		++ioc.count;
		p.SendHtml(p.prefixHtml, "<html><body>home!!!</body></html>");
		return 0;
	});

	// 起一个协程来实现 创建一份 client 并 自动连接到 server
	co_spawn(ioc, [&]()->awaitable<void> {
		// 每秒显示一次 QPS
		while (!ioc.stopped()) {
			ioc.count = 0;
			co_await xx::Timeout(1000ms);
			std::cout << "***** QPS = " << ioc.count << std::endl;
		}
	}, detached);

	ioc.run();
	return 0;
}
