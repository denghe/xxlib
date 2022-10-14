#pragma execution_character_set("utf-8")
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
		p.OutOnce<xx::HtmlHeaders::OK_200_Html>(R"(<!DOCTYPE html>
<html>
	<body>
		<form action='/name' method='post'>
			name: <input type='text' name='name'>
			repeat: <input type='number' min='1' max='100' name='repeat_times'>
			<br><input type='submit' value='submit'>
		</form>
	</body>
</html>)"sv);
		return 0;
	});

	SHPeer::RegisterHttpRequestHandler("/name"sv, [&](SHPeer& p)->int {
		// 将 "key=value&..." 参数读到变量( 带类型转换 )
		auto [name, repeat_times] = p.GetArgsArray("name"sv, "repeat_times"sv).ToTuple<std::string_view, std::optional<size_t>>();
		// 检查参数是否有异常?
		if (name.empty() || !repeat_times.has_value()) {
			p.OutOnce<xx::HtmlHeaders::OK_200_Text>("参数有问题! 请返回上页重新填"sv);
		}
		else {
			// 开始流式拼接输出内容
			p.OutBegin<xx::HtmlHeaders::OK_200_Html>();

			p.Out("<!DOCTYPE html><html><head><meta charset='utf-8'><title>hello title</title></head><body>"sv);
			for (size_t i = 0; i < repeat_times; i++) {
				char buf[32];
				p.Out("<a href='/'>hi!&nbsp;"sv, name, "&nbsp;"sv, xx::ToStringView(i, buf), "</a><br>"sv);
			}
			p.Out("<pre>"sv);
			xx::HttpEncodePreTo(p.out, R"(
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
)"sv);
			p.Out("</pre>"sv);
			p.Out("</body></html>"sv);

			// 结束拼接并发送
			p.OutEnd();
		}
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
