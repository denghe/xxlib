#pragma region includes
#include <asio.hpp>
using namespace std::literals;
using namespace std::literals::chrono_literals;
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
using asio::co_spawn;
using asio::awaitable;
using asio::detached;
#include <deque>
#include <string>
#include <unordered_set>
#pragma endregion

struct Listener : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;
	Listener() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

	template<typename SocketHandler>
	void Listen(uint16_t const& port, SocketHandler&& sh) {
		co_spawn(ioc, [this, port, sh = std::move(sh)]()->awaitable<void> {
			asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });
			acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			for (;;) {
				asio::ip::tcp::socket socket(ioc);
				if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
					socket.set_option(asio::ip::tcp::no_delay(true));
					sh(std::move(socket));
				}
			}
		}, detached);
	}
};

struct Server;
struct Peer : std::enable_shared_from_this<Peer> {
	asio::io_context& ioc;
	asio::ip::tcp::socket socket;
	asio::steady_timer writeBlocker;
	std::deque<std::vector<char>> writeQueue;
	bool stoped = true;
	Peer(asio::io_context& ioc_, asio::ip::tcp::socket&& socket_) : ioc(ioc_), socket(std::move(socket_)), writeBlocker(ioc_) {}

	void Start() {
		if (!stoped) return;
		stoped = false;
		writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
		writeQueue.clear();
		co_spawn(ioc, [self = shared_from_this()]{ return self->Read(); }, detached);
		co_spawn(ioc, [self = shared_from_this()]{ return self->Write(); }, detached);
	}

	void Stop() {
		if (stoped) return;
		stoped = true;
		socket.close();
		writeBlocker.cancel();
	}

	void Send(std::vector<char>&& buf) {
		if (stoped) return;
		writeQueue.emplace_back(std::move(buf));
		writeBlocker.cancel_one();
	}

	void HandleMessage(std::vector<char>&& buf) {
		// switch case decode xxxxxxxx
	}

protected:
	awaitable<void> Read() {
		for (;;) {
			std::vector<char> buf;
			{
				// 读 2 字节 包头
				uint16_t dataLen = 0;
				auto [ec, n] = co_await asio::async_read(socket, asio::buffer(&dataLen, sizeof(dataLen)), use_nothrow_awaitable);
				if (ec || !n) break;
				if (stoped) co_return;
				buf.resize(n);
			}
			for (;;) {
				// 读 数据
				auto [ec, n] = co_await asio::async_read(socket, asio::buffer(buf.data(), buf.size()), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
			}
			HandleMessage(std::move(buf));
			if (stoped) co_return;
		}
		Stop();
	}

	awaitable<void> Write() {
		while (socket.is_open()) {
			if (writeQueue.empty()) {
				co_await writeBlocker.async_wait(use_nothrow_awaitable);
				if (stoped) co_return;
			}
			auto& msg = writeQueue.front();
			auto buf = msg.data();
			auto len = msg.size();
		LabBegin:
			auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
			if (ec) break;
			if (stoped) co_return;
			if (n < len) {
				len -= n;
				buf += n;
				goto LabBegin;
			}
			writeQueue.pop_front();
		}
		Stop();
	}
};

struct Server : Listener {
	std::unordered_set<std::shared_ptr<Peer>> ps;
};


int main() {
	Server s;
	s.Listen(12345, [&](asio::ip::tcp::socket&& socket) {
		auto p = std::make_shared<Peer>(s.ioc, std::move(socket));
		s.ps.insert(p);
		p->Start();
	});
	s.ioc.run();
	return 0;
}
