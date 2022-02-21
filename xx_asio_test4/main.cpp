#include <asio.hpp>
using namespace std::chrono_literals;

struct Worker : public asio::noncopyable {
	Worker()
		: _ioc(1)
		, _guard(asio::make_work_guard(_ioc))
		, _thread(std::thread([this]() { _ioc.run(); }))
	{}

	~Worker() {
		_ioc.stop();
		if (_thread.joinable()) {
			_thread.join();
		}
	}

	asio::io_context _ioc;
	asio::executor_work_guard<asio::io_context::executor_type> _guard;  // keep ioc run
	std::thread _thread;
};

asio::awaitable<void> echo(asio::ip::tcp::socket socket) {
	char data[2048];
	try {
		for (;;) {
			auto n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
			co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
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
			try {
				auto& ioc = workers[i]._ioc;
				asio::ip::tcp::socket socket(ioc);
				co_await acceptor.async_accept(socket, asio::use_awaitable);
				asio::co_spawn(ioc, echo(std::move(socket)), asio::detached);
			}
			catch (std::exception& e) {
				std::printf("listener Exception: %s\n", e.what());

			}
		}
	}
}

int main() {
	try {
		auto workers_count = 16;
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
