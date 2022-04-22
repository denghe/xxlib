// asio c++20 gateway
// 总体架构为 伪多线程（ 独立线程负责 accept, socket 均匀分布到多个线程，每个线程逻辑完整独立，互相不通信，自己连别的服务，不共享 )

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
#include <unordered_set>
#include <deque>
#include <iostream>

/***********************************************************************************************************/
// utils
/***********************************************************************************************************/

asio::awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

std::string ToString(asio::ip::tcp::socket const& sock) {
	auto ep = sock.remote_endpoint();
	return ep.address().to_string() + ":" + std::to_string(ep.port());
}

void ToString(std::string& s, asio::ip::tcp::socket const& sock) {
	auto ep = sock.remote_endpoint();
	s = ep.address().to_string() + ":" + std::to_string(ep.port());
}

/***********************************************************************************************************/
// PeerBase
/***********************************************************************************************************/

struct PeerBase : asio::noncopyable {
	virtual ~PeerBase() {}
	virtual void Deliver(std::string && msg) = 0;
	virtual void Deliver(std::string const& msg) = 0;
};
typedef std::shared_ptr<PeerBase> PeerBase_p;


/***********************************************************************************************************/
// Worker
/***********************************************************************************************************/

struct Worker : asio::noncopyable {
	asio::io_context ioc;
	std::thread thread;

	// logic contents
	std::unordered_set<PeerBase_p> peers;
	std::deque<std::string> messages;

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

struct Workers : asio::noncopyable {
	uint32_t cap;
	std::unique_ptr<Worker[]> workers;

	Workers(uint32_t cap_ = 0) {
		cap = cap_ ? cap_ : 1;//std::thread::hardware_concurrency() * 2;
		workers = std::make_unique<Worker[]>(cap);
	}

	Worker& operator[](size_t const& i) const {
		assert(i < cap);
		return workers[i];
	}
};

/***********************************************************************************************************/
// Server
/***********************************************************************************************************/

struct Server : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;
	Workers workers;

	Server()
		: ioc(1)
		, signals(ioc, SIGINT, SIGTERM)
	{
		asio::co_spawn(ioc, Listen(55555), asio::detached);
	}

	void Run() {
		ioc.run();
	}

	asio::awaitable<void> Listen(uint16_t port);
};

/***********************************************************************************************************/
// Peer
/***********************************************************************************************************/

struct Peer : PeerBase, std::enable_shared_from_this<Peer> {
	asio::io_context& ioc;
	std::unordered_set<PeerBase_p>& peers;
	std::deque<std::string>& messages;

	asio::ip::tcp::socket socket;
	asio::steady_timer timer;
	std::string addr;
	std::deque<std::string> writeQueue;

	Peer(Worker& w, asio::ip::tcp::socket&& socket_)
		: ioc(w.ioc)
		, peers(w.peers)
		, messages(w.messages)
		, socket(std::move(socket_))
		, timer(ioc)
	{
		timer.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void Start() {
		ToString(addr, socket);
		std::cout << addr << " accepted." << std::endl;
		peers.insert(shared_from_this());
		for (auto const& msg : messages) {
			Deliver(msg);
		}
		asio::co_spawn(ioc, [self = shared_from_this()]{ return self->Read(); }, asio::detached);
		asio::co_spawn(ioc, [self = shared_from_this()]{ return self->Write(); }, asio::detached);
	}

	void Stop() {
		peers.erase(shared_from_this());
		socket.close();
		timer.cancel();
	}

	virtual void Deliver(std::string && msg) override {
		writeQueue.push_back(std::move(msg));
		timer.cancel_one();
	}

	virtual void Deliver(std::string const& msg) override {
		writeQueue.push_back(msg);
		timer.cancel_one();
	}

protected:
	asio::awaitable<void> Read() {
		try {
			for (std::string buf;;) {
				auto n = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf, 1024), "\n", asio::use_awaitable);
				auto msg = buf.substr(0, n);
				messages.push_back(msg);
				for (auto p : peers) {
					p->Deliver(msg);
				}
				buf.erase(0, n);
			}
		}
		catch (std::exception&) {
			Stop();
		}
	}

	asio::awaitable<void> Write() {
		try {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					asio::error_code ec;
					co_await timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
				}
				else {
					co_await asio::async_write(socket, asio::buffer(writeQueue.front()), asio::use_awaitable);
					writeQueue.pop_front();
				}
			}
		}
		catch (std::exception&) {
			Stop();
		}
	}
};


asio::awaitable<void> Server::Listen(uint16_t port) {
	asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
	for (;;) {
		for (uint32_t i = 0; i < workers.cap; ++i) {
			auto& w = workers[i];
			try {
				asio::ip::tcp::socket socket(w.ioc);
				co_await acceptor.async_accept(socket, asio::use_awaitable);
				std::make_shared<Peer>(w, std::move(socket))->Start();
			}
			catch (std::exception& e) {
				std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
			}
		}
	}
}


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
