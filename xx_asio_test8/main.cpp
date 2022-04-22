// asio c++20 gateway

/***********************************************************************************************************/
// includes
/***********************************************************************************************************/

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
#include <chrono>
using namespace std::literals::chrono_literals;
#include <iostream>

/***********************************************************************************************************/
// timeout
/***********************************************************************************************************/

asio::awaitable<void> timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

/***********************************************************************************************************/
// Worker
/***********************************************************************************************************/

struct Worker : public asio::noncopyable {
	Worker(Worker&&) noexcept = delete;
	Worker& operator=(Worker&&) noexcept = delete;

	asio::io_context ioc;
	std::thread thread;

	Worker()
		: ioc(1)
		, thread(std::thread([this]() { auto g = asio::make_work_guard(ioc); ioc.run(); }))
	{}

	~Worker() {
		ioc.stop();
		if (thread.joinable()) {
			thread.join();
		}
	}
};

/***********************************************************************************************************/
// Workers
/***********************************************************************************************************/

struct Workers : public asio::noncopyable {
	Workers(Workers&&) noexcept = default;
	Workers& operator=(Workers&&) noexcept = default;

	uint32_t size;
	std::unique_ptr<Worker[]> workers;

	Workers() 
		: size(std::thread::hardware_concurrency())
		, workers(std::make_unique<Worker[]>(size))
	{}

	Worker& operator[](size_t const& i) const {
		assert(i < size);
		return workers[i];
	}
};

/***********************************************************************************************************/
// Server
/***********************************************************************************************************/

struct Server : public asio::noncopyable {
	Server(Server&&) noexcept = default;
	Server& operator=(Server&&) noexcept = default;

	Workers workers;
	asio::io_context ioc;
	asio::signal_set signals;

	Server()
		: ioc(1)
		, signals(ioc, SIGINT, SIGTERM)
	{
		asio::co_spawn(ioc, Listen(55555), asio::detached);
	}

	void Run() {
		ioc.run();
	}

	asio::awaitable<void> Listen(uint16_t port) {
		asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });

		for (;;) {
			for (int i = 0; i < workers.size; ++i) {
				auto& w = workers[i];
				try {
					asio::ip::tcp::socket socket(w.ioc);
					co_await acceptor.async_accept(socket, asio::use_awaitable);
					asio::co_spawn(w.ioc, Echo(std::move(socket)), asio::detached);
				}
				catch (std::exception& e) {
					std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
				}
			}
		}
	}

	asio::awaitable<void> Echo(asio::ip::tcp::socket socket) {
		std::string addr;
		try {
			auto ep = socket.remote_endpoint();
			addr = ep.address().to_string() + ":" + std::to_string(ep.port());
			std::cout << addr << " accepted." << std::endl;

			char data[2048];
			for (;;) {
				auto n = co_await socket.async_read_some(asio::buffer(data), asio::use_awaitable);
				co_await asio::async_write(socket, asio::buffer(data, n), asio::use_awaitable);
			}
		}
		catch (std::exception& e) {
			std::cout << addr << " echo Exception : " << e.what() << std::endl;
		}
	}
};


/***********************************************************************************************************/
// main
/***********************************************************************************************************/

int main() {
	try {
		Server server;
		server.Run();
	}
	catch (std::exception& e) {
		std::cout << "main() Exception : " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
