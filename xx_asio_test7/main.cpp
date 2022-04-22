// asio c++20 lobby
// 总体架构为 单线程, 和 gateway 1 对 多

#include <asio.hpp>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <iostream>
#include <chrono>
using namespace std::literals::chrono_literals;

/***********************************************************************************************************/
// PeerBase

struct PeerBase : asio::noncopyable {
	virtual ~PeerBase() {}
};
typedef std::shared_ptr<PeerBase> PeerBase_p;

/***********************************************************************************************************/
// Server

struct Server : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;

	size_t peerAutoId = 0;
	std::unordered_map<size_t, std::shared_ptr<PeerBase>> peers;

	Server()
		: ioc(1)
		, signals(ioc, SIGINT, SIGTERM)
	{
	}

	void Run() {
		ioc.run();
	}

	template<typename PeerType>
	void Listen(uint16_t port) {
		asio::co_spawn(ioc, ListenCore<PeerType>(port), asio::detached);
	}

	template<typename PeerType>
	asio::awaitable<void> ListenCore(uint16_t port) {
		asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
		for (;;) {
			try {
				asio::ip::tcp::socket socket(ioc);
				co_await acceptor.async_accept(socket, asio::use_awaitable);
				std::make_shared<PeerType>(*this, std::move(socket))->Start();
			}
			catch (std::exception& e) {
				std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
			}
		}
	}
};

/***********************************************************************************************************/
// Peer

struct Peer : PeerBase, std::enable_shared_from_this<Peer> {
	Server& server;

	size_t id;
	asio::ip::tcp::socket socket;
	asio::steady_timer timeouter;
	asio::steady_timer writeBarrier;
	std::deque<std::string> writeQueue;

	std::string addr;
	bool stoping = false;
	bool stoped = false;

	Peer(Server& server_, asio::ip::tcp::socket&& socket_)
		: server(server_)
		, id(++server_.peerAutoId)
		, socket(std::move(socket_))
		, writeBarrier(server_.ioc)
		, timeouter(server_.ioc)
	{
		writeBarrier.expires_at(std::chrono::steady_clock::time_point::max());
		ResetTimeout(5s);
	}

	void Start() {
		server.peers.insert({ id, shared_from_this() });	// 容器持有

		auto ep = socket.remote_endpoint();
		addr = ep.address().to_string() + ":" + std::to_string(ep.port());
		std::cout << addr << " accepted." << std::endl;

		// 启动读写协程( 顺便持有. 协程退出时可能触发析构 )
		asio::co_spawn(server.ioc, [self = shared_from_this()]{ return self->Read(); }, asio::detached);
		asio::co_spawn(server.ioc, [self = shared_from_this()]{ return self->Write(); }, asio::detached);
	}

	void Stop() {
		if (stoped) return;
		stoped = true;

		socket.close();
		writeBarrier.cancel();
		timeouter.cancel();

		assert(server.peers.contains(id));

		// 可能触发析构, 写在最后面, 避免访问成员失效
		server.peers.erase(id);
	}

	void DelayStop(std::chrono::steady_clock::duration const& d) {
		if (stoping || stoped) return;
		stoping = true;

		std::cout << addr << " stoping..." << std::endl;
		ResetTimeout(d);
	}

	void ResetTimeout(std::chrono::steady_clock::duration const& d) {
		timeouter.expires_from_now(d);
		timeouter.async_wait([this](auto&& ec) {
			if (!ec) {
				Stop();
			}
		});
	}

	void Send(std::string&& msg) {
		writeQueue.push_back(std::move(msg));
		writeBarrier.cancel_one();
	}

	void Send(std::string const& msg) {
		writeQueue.push_back(msg);
		writeBarrier.cancel_one();
	}

	virtual int HandleMessage(std::string msg) = 0;

protected:
	asio::awaitable<void> Read() {
		try {
			for (std::string buf;;) {
				auto n = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf, 1024), " ", asio::use_awaitable);
				if (stoping || stoped) break;

				if (int r = HandleMessage(buf.substr(0, n - 1))) {
					std::cout << addr << " HandleMessage r = " << r << std::endl;
					break;
				}

				buf.erase(0, n);
			}
		}
		catch (std::exception& ec) {
			std::cout << addr << " Read() ec = " << ec.what() << std::endl;
			Stop();
		}
		std::cout << addr << " Read() end." << std::endl;
	}

	asio::awaitable<void> Write() {
		try {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					asio::error_code ec;
					co_await writeBarrier.async_wait(asio::redirect_error(asio::use_awaitable, ec));
				}
				else {
					co_await asio::async_write(socket, asio::buffer(writeQueue.front()), asio::use_awaitable);
					writeQueue.pop_front();
				}
				if (stoped) break;
			}
		}
		catch (std::exception& ec) {
			std::cout << addr << " Write() ec = " << ec.what() << std::endl;
			Stop();
		}
		std::cout << addr << " Write() end." << std::endl;
	}
};


struct MyPeer : Peer {
	using Peer::Peer;
	virtual int HandleMessage(std::string msg) override {
		if (msg == "1") {
			ResetTimeout(10s);
			Send(std::string("peer id = ") + std::to_string(id) + "\r\n");
		}
		else if (msg == "2") {
			DelayStop(3s);
			Send("DelayStop(3s)\r\n");
		}
		else {
			std::cout << addr << " recv unhandled msg = " << msg << std::endl;
		}
		return 0;
	}
};


/***********************************************************************************************************/
// main

int main() {
	try {
		Server server;
		server.Listen<MyPeer>(55555);
		server.Run();
	}
	catch (std::exception& e) {
		std::cout << "main() Exception : " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
