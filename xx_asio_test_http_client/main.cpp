#include <xx_asio_tcp_base_http.h>

// asio io_context 上下文( 生命周期最久的 全局交互 & 胶水 容器 )
struct IOC : xx::IOCBase {
	using IOCBase::IOCBase;
};

// client http peer
struct CHPeer : xx::PeerTcpBaseCode<CHPeer, IOC>, xx::PeerHttpCode<CHPeer, xx::HttpType::Response> {
	using PeerTcpBaseCode::PeerTcpBaseCode;

	inline static constexpr std::string_view reqStr =
		"GET / HTTP/1.1\r\n"
		"Host: www.kittyhell.com\r\n"
		"User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "
		"Pathtraq/0.9\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"
		"Accept-Encoding: gzip,deflate\r\n"
		"Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"
		"Keep-Alive: 115\r\n"
		"Connection: keep-alive\r\n"
		"Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "
		"__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "
		"__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"
		"\r\n"sv;

	// package cache for send
	xx::DataShared reqPkg;

	void SendPkg() {
		Send(reqPkg);
	}

	void Start_() {
		std::cout << "***** connected" << std::endl;
		// fill cache package
		xx::Data d;
		d.WriteBuf(reqStr.data(), reqStr.size());
		reqPkg = xx::DataShared(std::move(d));

		Send(reqPkg);
	}

	void Stop_() {
		std::cout << "***** disconnected" << std::endl;
	}

	// 处理收到的 http 请求
	int ReceiveHttp() {
		//std::cout << GetDumpStr() << std::endl;				// 打印一下收到的东西
		// todo
		Send(reqPkg);
		return 0;
	}
};

int Run() {
	IOC ioc(1);

	for (size_t i = 0; i < 2; i++) {
		// 起一个协程来实现 创建一份 client 并 自动连接到 server
		co_spawn(ioc, [&]()->awaitable<void> {
			// 创建 client peer
			auto cp = xx::Make<CHPeer>(ioc);

			// 反复测试，不退出
			while (!ioc.stopped()) {
				// 如果已断开 就尝试重连
				if (cp->IsStoped()) {
					std::cout << "***** connecting..." << std::endl;
					co_await cp->Connect(asio::ip::address::from_string("127.0.0.1"), 12345);
				}
				// 频率控制在 1 秒 2 次
				co_await xx::Timeout(500ms);
			}
		}, detached);
	}

	ioc.run();
	return 0;
}

int main() {
	std::array<std::thread, 1> ts;
	for (auto& t : ts) {
		t = std::thread([] { Run(); });
		t.detach();
	}
	std::cin.get();
}
