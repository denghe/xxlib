// asio c++20 lobby

/******************************************************************************************************************************/
// xx_asio
/******************************************************************************************************************************/

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
#include <chrono>
using namespace std::literals::chrono_literals;
#include <iostream>


asio::awaitable<void> timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

asio::awaitable<void> logic(asio::io_context& ioc) {
LabBegin:
	{
		std::cout << "begin resolve" << std::endl;

		asio::ip::tcp::resolver resolver(ioc);
		auto r1 = co_await(resolver.async_resolve("127.0.0.1", "55555", use_nothrow_awaitable) || timeout(5s));
		if (r1.index()) {
			std::cout << "resolve timeout" << std::endl;
			goto LabEnd;
		}
		auto& [e1, rs] = std::get<0>(r1);
		if (e1) {
			std::cout << "resolve error = " << e1 << std::endl;
			goto LabEnd;
		}
		else {
			std::cout << "resolve success. ip list = {" << std::endl;
			for (auto const& r : rs) {
				std::cout << "    " << r.endpoint() << std::endl;
			}
			std::cout << "}" << std::endl;
		}

		std::cout << "begin connect" << std::endl;

		// todo: 同时连多个 ip. 保留先连上的

		asio::ip::tcp::socket s(ioc);
		auto r2 = co_await(s.async_connect(*rs.begin(), use_nothrow_awaitable) || timeout(5s));
		if (r2.index()) {
			std::cout << "connect timeout" << std::endl;
			goto LabEnd;
		}
		auto& [e2] = std::get<0>(r2);
		if (e2) {
			std::cout << "connect error = " << e2 << std::endl;
			goto LabEnd;
		}

		std::cout << "begin w + r" << std::endl;

		while (true) {
			auto str = "a";
			auto strLen = 1;
			auto r3 = co_await(asio::async_write(s, asio::buffer(str, strLen), use_nothrow_awaitable) || timeout(5s));
			if (r3.index()) {
				std::cout << "write timeout" << std::endl;
				goto LabEnd;
			}
			auto& [e3, wsiz] = std::get<0>(r3);
			if (e3) {
				std::cout << "write error = " << e3 << std::endl;
				goto LabEnd;
			}
			if (wsiz < strLen) {
				std::cout << "write failed. wsiz = " << wsiz << ", strLen = " << strLen << std::endl;
				goto LabEnd;
			}

			char data;
			auto dataLen = 1;
			auto r4 = co_await(s.async_read_some(asio::buffer(&data, dataLen), use_nothrow_awaitable) || timeout(5s));
			if (r4.index()) {
				std::cout << "read timeout" << std::endl;
				goto LabEnd;
			}
			auto& [e4, rsiz] = std::get<0>(r4);
			if (e4) {
				std::cout << "read error = " << e4 << std::endl;
				goto LabEnd;
			}
			if (rsiz < dataLen) {
				std::cout << "read failed. rsiz = " << rsiz << std::endl;
				goto LabEnd;
			}

			if (str[0] != data) {
				std::cout << "warning: read bad data. str[0] = " << (int)str[0] << ", data = " << (int)data << std::endl;
			}
		}
	}
LabEnd:
	co_await timeout(1s);
	goto LabBegin;
}

double NowMS() {
	return std::chrono::system_clock::now().time_since_epoch().count() / 10000.;
}




int main() {
	asio::io_context ioc(1);
	auto&& iocGuard = asio::make_work_guard(ioc);

	asio::co_spawn(ioc, logic(ioc), asio::detached);

	auto ms = NowMS();
	while (true) {
		std::this_thread::sleep_for(500ms);			// 模拟游戏每帧来一发
		std::cout << "------------------------------------------------- frame ms = " << (NowMS() - ms) << std::endl;
		ioc.poll_one();
	}
	return 0;
}

