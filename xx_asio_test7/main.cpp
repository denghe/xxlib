// asio c++20 lobby
// 总体架构为 单线程, 和 gateway 1 对 多

#include <asio.hpp>
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
				std::make_shared<PeerType>((decltype(PeerType::server))*this, std::move(socket))->Start();
			}
			catch (std::exception& e) {
				std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
			}
		}
	}
};

/***********************************************************************************************************/
// Peer

template<typename ServerType>
struct Peer : PeerBase, std::enable_shared_from_this<Peer<ServerType>> {
	ServerType& server;

	size_t id;
	asio::ip::tcp::socket socket;
	asio::steady_timer timeouter;
	asio::steady_timer writeBarrier;
	std::deque<std::string> writeQueue;

	std::string addr;
	bool stoping = false;
	bool stoped = false;

	Peer(ServerType& server_, asio::ip::tcp::socket&& socket_)
		: server(server_)
		, id(++server_.peerAutoId)
		, socket(std::move(socket_))
		, writeBarrier(server_.ioc)
		, timeouter(server_.ioc)
	{
		writeBarrier.expires_at(std::chrono::steady_clock::time_point::max());
		ResetTimeout(20s);
	}

	// 由于 shared_from_this 无法在构造函数中使用，故拆分出来。平时不可以调用
	void Start() {
		server.peers.insert(std::make_pair(id, this->shared_from_this()));	// 容器持有

		auto ep = socket.remote_endpoint();
		addr = ep.address().to_string() + ":" + std::to_string(ep.port());
		std::cout << addr << " accepted." << std::endl;

		// 启动读写协程( 顺便持有. 协程退出时可能触发析构 )
		asio::co_spawn(server.ioc, [self = this->shared_from_this()]{ return self->Read(); }, asio::detached);
		asio::co_spawn(server.ioc, [self = this->shared_from_this()]{ return self->Write(); }, asio::detached);
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
		ResetTimeout(d);
		stoping = true;
		std::cout << addr << " stoping..." << std::endl;
	}

	void ResetTimeout(std::chrono::steady_clock::duration const& d) {
		timeouter.expires_at(std::chrono::steady_clock::now() + d);
		timeouter.async_wait([this](auto&& ec) {
			if (ec) return;
			Stop();
		});
	}

	void Send(std::string&& msg) {
		if (stoped) return;
		writeQueue.push_back(std::move(msg));
		writeBarrier.cancel_one();
	}

	void Send(std::string const& msg) {
		if (stoped) return;
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

/***********************************************************************************************************/
// logic

#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

asio::awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

struct MyPeer;
struct MyServer : Server {
	std::unordered_map<std::string, std::function<int(MyPeer&)>> msgHandlers;
	MyServer();
};
using MyServerPeer = Peer<MyServer>;

struct MyPeer : MyServerPeer {
	using MyServerPeer::MyServerPeer;
	virtual int HandleMessage(std::string msg) override {
		if (auto iter = server.msgHandlers.find(msg); iter != server.msgHandlers.end()) return iter->second(*this);
		std::cout << addr << " Peer1 recv unhandled msg = " << msg << std::endl;
		return 0;
	}
};
MyServer::MyServer() {
	msgHandlers.insert({ "1", [this](MyPeer& p) {
		p.Send("1 = timeout 20s\r\n");
		p.ResetTimeout(20s);
		return 0;
	} });
	msgHandlers.insert({ "2", [this](MyPeer& p) {
		p.Send("2 = DelayStop 3s\r\n");
		p.DelayStop(3s);
		return 0;
	} });
	msgHandlers.insert({ "3", [this](MyPeer& p) { 
		asio::co_spawn(ioc, [this, p = p.shared_from_this()]()->asio::awaitable<void> {
			for (int i = 5; i >= 0; --i) {
				p->Send(std::to_string(i) + "\r\n");
				co_await Timeout(1s);
			}
			p->Stop();
			co_return;
		}, asio::detached);
		return 0;
	} });
}

int main() {
	try {
		MyServer server;
		server.Listen<MyPeer>(55551);
		server.Run();
	}
	catch (std::exception& e) {
		std::cout << "main() Exception : " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
