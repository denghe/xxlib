#include <xx_asio_tcp_base_http.h>

// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
};

// client http peer
struct CHPeer : xx::PeerTcpBaseCode<CHPeer, IOC, 100> , xx::PeerHttpCode<CHPeer, 1024 * 32> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	void Start_() {
		// todo: 起协程开始测试
	}

	// 处理收到的 http 请求
	int ReceiveHttpRequest() {
		// todo
		return 0;
	}
};

int main() {
	IOC ioc(1);

	// 起一个协程来实现 创建一份 client 并 自动连接到 server
	co_spawn(ioc, [&]()->awaitable<void> {
		// 创建 client peer
		auto cp = xx::Make<CHPeer>(ioc);

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
