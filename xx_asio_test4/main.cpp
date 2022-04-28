#include <asio.hpp>
#include <iostream>

using namespace std::chrono_literals;
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

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "args   =   port" << std::endl;
        return 0;
    }

	std::thread t{ [&] {
		while (true) {
			counter = 0;
			std::this_thread::sleep_for(1s);
			auto c = (int64_t)counter;
			if (c < 0) break;
			std::cout << "qps = " << c << std::endl;
		}
	} };
	t.detach();

	try {
		auto workers_count = std::thread::hardware_concurrency();
		auto workers = std::make_unique<Worker[]>(workers_count);

		asio::io_context ioc(1);
		asio::signal_set signals(ioc, SIGINT, SIGTERM);
		signals.async_wait([&](auto, auto) { ioc.stop(); });

        auto port = std::strtol(argv[1], nullptr, 10);

		asio::co_spawn(ioc, listener(port, workers.get(), workers_count), asio::detached);

		ioc.run();
	}
	catch (std::exception& e) {
		std::printf("main Exception: %s\n", e.what());
		return -1;
	}
	return 0;
}























//#include <asio.hpp>
//#include <iostream>
//#include <queue>
//#include <chrono>
//#include <atomic>
//using namespace std::chrono_literals;
//std::atomic_int64_t counter;
//
//struct Worker : public asio::noncopyable {
//	Worker()
//		: _ioc(1)
//		, _thread(std::thread([this]() { auto g = asio::make_work_guard(_ioc); _ioc.run(); }))
//	{}
//
//	~Worker() {
//		_ioc.stop();
//		if (_thread.joinable()) {
//			_thread.join();
//		}
//	}
//
//	asio::io_context _ioc;
//	std::thread _thread;
//};
//
//struct buffer
//{
//	size_t size_ = 0;
//	std::array<char, 2048> v;
//
//	char* data()
//	{
//		return v.data();
//	}
//
//	size_t size() const
//	{
//		return size_;
//	}
//};
//
//struct socket_send_queue
//{
//	bool sending = false;
//	std::vector<std::unique_ptr<buffer>> pool;
//	std::queue<std::unique_ptr<buffer>> wq;
//
//	std::unique_ptr<buffer> make_buffer()
//	{
//		std::unique_ptr<buffer> v;
//		if (pool.empty())
//		{
//			v = std::make_unique<buffer>();
//		}
//		else
//		{
//			v = std::move(pool.back());
//			pool.pop_back();
//		}
//		return v;
//	}
//};
//
//void post_send(socket_send_queue& q, asio::ip::tcp::socket& socket)
//{
//	if (q.sending || q.wq.empty())
//		return;
//	q.sending = true;
//	auto& wb = q.wq.front();
//	auto b = asio::buffer(wb->data(), wb->size());
//
//	asio::async_write(
//		socket,
//		b,
//		[&q, &socket](const asio::error_code& e, std::size_t) mutable
//		{
//			q.sending = false;
//			q.pool.emplace_back(std::move(q.wq.front()));
//			q.wq.pop();
//			if (!e)
//			{
//				post_send(q, socket);
//			}
//			else
//			{
//				std::printf("echo Exception: %s\n", e.message().data());
//			}
//		});
//}
//
//asio::awaitable<void> echo(asio::ip::tcp::socket socket) {
//	try {
//
//		socket_send_queue ssq;
//		for (;;) {
//
//			auto rb = ssq.make_buffer();
//			auto n = co_await socket.async_read_some(asio::buffer(rb->data(), 2048), asio::use_awaitable);
//			rb->size_ = n;
//			ssq.wq.emplace(std::move(rb));
//			post_send(ssq, socket);
//			++counter;
//		}
//	}
//	catch (std::exception& e) {
//		std::printf("echo Exception: %s\n", e.what());
//	}
//}
//
//asio::awaitable<void> listener(uint16_t port, Worker* workers, int workers_count) {
//	auto executor = co_await asio::this_coro::executor;
//	asio::ip::tcp::acceptor acceptor(executor, { asio::ip::tcp::v4(), port });
//	for (;;) {
//		for (int i = 0; i < workers_count; ++i) {
//			auto& w = workers[i];
//			try {
//				asio::ip::tcp::socket socket(w._ioc);
//				co_await acceptor.async_accept(socket, asio::use_awaitable);
//				std::cout << i << ", " << socket.local_endpoint() << std::endl;
//				asio::co_spawn(w._ioc, echo(std::move(socket)), asio::detached);
//			}
//			catch (std::exception& e) {
//				std::printf("listener Exception: %s\n", e.what());
//			}
//		}
//	}
//}
//
//int main() {
//	std::thread t{ [&] {
//		while (true) {
//			counter = 0;
//			std::this_thread::sleep_for(1s);
//			auto c = (int64_t)counter;
//			if (c < 0) break;
//			std::printf("qps = %lld\n", c);
//		}
//	} };
//	t.detach();
//
//	try {
//		auto workers_count = std::thread::hardware_concurrency();
//		auto workers = std::make_unique<Worker[]>(workers_count);
//
//		asio::io_context ioc(1);
//		asio::signal_set signals(ioc, SIGINT, SIGTERM);
//		signals.async_wait([&](auto, auto) { ioc.stop(); });
//
//		asio::co_spawn(ioc, listener(55555, workers.get(), workers_count), asio::detached);
//
//		ioc.run();
//	}
//	catch (std::exception& e) {
//		std::printf("main Exception: %s\n", e.what());
//		return -1;
//	}
//	return 0;
//}



