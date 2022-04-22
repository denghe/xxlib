// asio c++20 lobby
// 总体架构为 单线程, 和 gateway 1 对 多

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
#include <unordered_map>
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
	virtual void Send(std::string&& msg) = 0;
	virtual void Send(std::string const& msg) = 0;
};
typedef std::shared_ptr<PeerBase> PeerBase_p;

/***********************************************************************************************************/
// Server
/***********************************************************************************************************/

struct Server : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;

	// logic contents
	size_t peerAutoId = 0;
	std::unordered_map<size_t, PeerBase_p> peers;

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
	Server& server;

	asio::ip::tcp::socket socket;
	size_t id;
	asio::steady_timer timer;
	std::string addr;
	std::deque<std::string> writeQueue;

	Peer(Server& server_, asio::ip::tcp::socket&& socket_)
		: server(server_)
		, id(++server_.peerAutoId)
		, socket(std::move(socket_))
		, timer(server_.ioc)
	{
		timer.expires_at(std::chrono::steady_clock::time_point::max());
	}

	void Start() {
		ToString(addr, socket);
		std::cout << addr << " accepted." << std::endl;
		server.peers.insert({ id, shared_from_this() });
		asio::co_spawn(server.ioc, [self = shared_from_this()]{ return self->Read(); }, asio::detached);
		asio::co_spawn(server.ioc, [self = shared_from_this()]{ return self->Write(); }, asio::detached);
		// todo: logic here
	}

	void Stop() {
		server.peers.erase(id);
		socket.close();
		timer.cancel();
		std::cout << addr << " stoped." << std::endl;
	}

	virtual void Send(std::string&& msg) override {
		writeQueue.push_back(std::move(msg));
		timer.cancel_one();
	}

	virtual void Send(std::string const& msg) override {
		writeQueue.push_back(msg);
		timer.cancel_one();
	}

protected:
	asio::awaitable<void> Read() {
		try {
			for (std::string buf;;) {
				auto n = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf, 1024), "\n", asio::use_awaitable);
				auto msg = buf.substr(0, n);

				std::cout << addr << " recv msg = " << msg << std::endl;
				if (msg == "id") {
					Send(std::to_string(id));
				}
				else if (msg == "hi") {
					Send("ni hao!");
				}
				// todo: 超时逻辑. 得测试是否能这么用，使用 expires_at 来续命
				//deadline_timer t;
				//t.expires_at(now + 10s)
				//	t.async_wait(kill logic)
				//	...
				//	t.expires_at(now + 10s)
				//	...

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
		try {
			asio::ip::tcp::socket socket(ioc);
			co_await acceptor.async_accept(socket, asio::use_awaitable);
			std::make_shared<Peer>(*this, std::move(socket))->Start();
		}
		catch (std::exception& e) {
			std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
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
