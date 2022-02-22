#include <asio.hpp>

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
	std::atomic_int _num;
};

asio::awaitable<void> echo(Worker& worker, asio::ip::tcp::socket socket) {
	++worker._num;
	try {
		char data[2048];
		for (;;) {
			auto n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
			co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
		}
	}
	catch (std::exception& e) {
		std::printf("echo Exception: %s\n", e.what());
	}
	--worker._num;
}

asio::awaitable<void> listener(uint16_t port, Worker* workers, int workers_count) {
	auto executor = co_await asio::this_coro::executor;
	asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), port });
	for (;;) {
		int i = 0, m = std::numeric_limits<int>::max();
		for (int j = 0; j < workers_count; ++j) {
			int n = workers[i]._num;
			if (n < m) {
				m = n;
				i = j;
			}
		}
		auto& w = workers[i];
		try {
			asio::ip::tcp::socket socket(w._ioc);
			co_await acceptor.async_accept(socket, asio::use_awaitable);
			asio::co_spawn(w._ioc, echo(w, std::move(socket)), asio::detached);
		}
		catch (std::exception& e) {
			std::printf("listener Exception: %s\n", e.what());

		}
	}
}

int main() {
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
