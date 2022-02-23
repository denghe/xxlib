#include <asio.hpp>

using namespace std::chrono_literals;
std::chrono::system_clock::time_point defaultBeginTime;
std::chrono::system_clock::time_point beginTime;
std::atomic_int64_t counter;

asio::awaitable<void> dialer(asio::io_context& ioc, std::string_view domain, std::string_view port) {
	try {
		asio::ip::tcp::resolver resolver(ioc);
		auto endpoints = co_await resolver.async_resolve(domain, port, asio::use_awaitable);

		asio::ip::tcp::socket socket(ioc);
		co_await socket.async_connect(*endpoints.begin(), asio::use_awaitable);

		char data;
		beginTime = std::chrono::system_clock::now();
		for (;;) {
			co_await asio::async_write(socket, asio::buffer("a", 1), asio::use_awaitable);
			auto n = co_await socket.async_read_some(asio::buffer(&data, 1), asio::use_awaitable);
			if (n != 1) {
				std::printf("dialer async_read_some n = %d\n", (int)n);
				break;
			}
			++counter;
		}
	}
	catch (std::exception& e) {
		std::printf("dialer Exception: %s\n", e.what());
	}
}

struct Client {
	int Run() {
		std::thread t{ [&] {
			try {
				asio::io_context ioc(1);
				asio::signal_set signals(ioc, SIGINT, SIGTERM);
				signals.async_wait([&](auto, auto) { ioc.stop(); });

				asio::co_spawn(ioc, dialer(ioc, "192.168.1.235", "55555"), asio::detached);

				ioc.run();
			}
			catch (std::exception& e) {
				std::printf("main Exception: %s\n", e.what());
			}
		} };
		t.detach();
	}
};

int main() {
	std::thread t{ [&] {
		while (true) {
			std::this_thread::sleep_for(1s);
			auto c = (int64_t)counter;
			counter = 0;
			std::printf("qps = %lld\n", c);
		}
	} };
	t.detach();

	auto clients = std::make_unique<Client[]>(16);
	return 0;
}
