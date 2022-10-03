#include <xx_asio_tcp_base_http.h>

// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
	ssize_t count = 0;
};

// server http peer
struct SHPeer : xx::PeerTcpBaseCode<SHPeer, IOC>, xx::PeerHttpCode<SHPeer>, xx::PeerHttpRequestHandlersCode<SHPeer> {
	using PeerTcpBaseCode::PeerTcpBaseCode;
};

int main() {
	IOC ioc(1);

	// 开始监听 http tcp 端口
	ioc.Listen(12345, [&](auto&& socket) {
		xx::Make<SHPeer>(ioc, std::move(socket))->Start();
	});
	std::cout << "***** http port: 12345" << std::endl;

	// 开始注册 http 处理函数
	SHPeer::RegisterHttpRequestHandler("/"sv, [&](SHPeer& p)->int {
#ifndef NDEBUG
        std::cout << p.GetDumpStr() << std::endl;
#endif
		++ioc.count;
		p.SendHtml<xx::HtmlHeaders::OK_200_Html>(R"(<html>
	<body>
		<form action='/name' method='post'>
			please input your name: <input type='text' name='name'>
			<br><input type='submit' value='submit'>
		</form>
	</body>
</html>)"sv);
		return 0;
	});

	SHPeer::RegisterHttpRequestHandler("/name"sv, [&](SHPeer& p)->int {
		p.SendHtml<xx::HtmlHeaders::OK_200_Html>(R"(<html>
	<body>
		<a href='/'>hi!)"sv, p.body, R"(</a>
	</body>
</html>)"sv);
		return 0;
	});

	// 起一个协程 每秒显示一次 QPS
	co_spawn(ioc, [&]()->awaitable<void> {
		while (!ioc.stopped()) {
			ioc.count = 0;
			co_await xx::Timeout(1000ms);
			if (ioc.count) {
				std::cout << "***** QPS = " << ioc.count << std::endl;
			}
		}
	}, detached);

	ioc.run();
	return 0;
}
