// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// db
#include "xx_asio_codes.h"
#include "pkg.h"

struct Worker : public asio::noncopyable {
	asio::io_context _ioc;
	std::thread _thread;
	// more db ctx here
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
};

struct LobbyPeer;
struct Game1Peer;
// ...
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;
	uint32_t numWorkers = std::thread::hardware_concurrency() * 2;
	std::unique_ptr<Worker[]> workers = std::make_unique<Worker[]>(numWorkers);

	std::shared_ptr<LobbyPeer> lobbyPeer;
	std::shared_ptr<Game1Peer> game1Peer;
	// ...
};

struct LobbyPeer : xx::PeerCode<LobbyPeer>, xx::PeerTimeoutCode<LobbyPeer>, xx::PeerRequestCode<LobbyPeer>, std::enable_shared_from_this<LobbyPeer> {
	Server& server;
	LobbyPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{}

	// 收到 请求( 返回非 0 表示失败，会 Stop )
	int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
		// todo: handle o_
		om.KillRecursive(o_);
		return 0;
	}

	// 收到 推送( 返回非 0 表示失败，会 Stop )
	int ReceivePush(xx::ObjBase_s&& o_) {
		// todo: handle o_
		om.KillRecursive(o_);
		return 0;
	}
};

struct Game1Peer : xx::PeerCode<Game1Peer>, xx::PeerTimeoutCode<Game1Peer>, xx::PeerRequestCode<Game1Peer>, std::enable_shared_from_this<Game1Peer> {
	Server& server;
	Game1Peer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{}

	// 收到 请求( 返回非 0 表示失败，会 Stop )
	int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
		// todo: handle o_
		om.KillRecursive(o_);
		return 0;
	}

	// 收到 推送( 返回非 0 表示失败，会 Stop )
	int ReceivePush(xx::ObjBase_s&& o_) {
		// todo: handle o_
		om.KillRecursive(o_);
		return 0;
	}
};

// ...

int main() {
	Server server;
	server.Listen<LobbyPeer>(55100);
	server.Listen<Game1Peer>(55101);
	// ...
	std::cout << "db running..." << std::endl;
	server.ioc.run();
	return 0;
}
