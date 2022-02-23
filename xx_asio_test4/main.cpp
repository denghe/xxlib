#include <asio.hpp>
#include <iostream>

using namespace std::chrono_literals;
std::chrono::system_clock::time_point defaultBeginTime;
std::chrono::system_clock::time_point beginTime;
std::atomic_int64_t counter;

struct Worker : public asio::noncopyable {
	Worker()
		: _ioc(1)
		, _thread(std::thread([this]() { auto g = asio::make_work_guard(_ioc); _ioc.run(); }))
	{}

	~Worker() {
		_ioc.stop();
		if (_thread.joinable()) {
			_thread.join();
		}
	}

	asio::io_context _ioc;
	std::thread _thread;
};

asio::awaitable<void> echo(asio::ip::tcp::socket socket) {
	try {
		char data[2048];
		for (;;) {
			auto n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
			co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
			++counter;
		}
	}
	catch (std::exception& e) {
		std::printf("echo Exception: %s\n", e.what());
	}
}

asio::awaitable<void> listener(uint16_t port, Worker* workers, int workers_count) {
	auto executor = co_await asio::this_coro::executor;
	asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), port });
	for (;;) {
		for (int i = 0; i < workers_count; ++i) {
			auto& w = workers[i];
			try {
				asio::ip::tcp::socket socket(w._ioc);
				co_await acceptor.async_accept(socket, asio::use_awaitable);
				std::cout << i << ", " << socket.local_endpoint() << std::endl;
				asio::co_spawn(w._ioc, echo(std::move(socket)), asio::detached);
			}
			catch (std::exception& e) {
				std::printf("listener Exception: %s\n", e.what());
			}
		}
	}
}

int main() {
	std::thread t{ [&] {
		while (true) {
			std::this_thread::sleep_for(1s);
			if (beginTime == defaultBeginTime) continue;
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - beginTime).count();
			if (ms == 0) continue;
			auto c = (int64_t)counter;
			if (c < 0) break;
			auto qps = (double)c / ((double)(ms) / 1000.);
			std::printf("qps = %f\n", qps);
		}
	} };
	t.detach();


	try {
		auto workers_count = std::thread::hardware_concurrency();
		auto workers = std::make_unique<Worker[]>(workers_count);

		asio::io_context ioc(1);
		asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { ioc.stop(); });

		asio::co_spawn(ioc, listener(55555, workers.get(), workers_count), asio::detached);

		ioc.run();
	}
	catch (std::exception& e) {
		std::printf("main Exception: %s\n", e.what());
		return -1;
	}
	return 0;
}
