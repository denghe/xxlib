// asio simple tcp server( xx::Data container )
#include "xx_asio_codes.h"
#include <pkg.h>

struct Server : xx::ServerCode<Server> {
	xx::ObjManager om;
};

struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, xx::PeerRequestCode<SPeer>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{}
	int ReceiveRequest(int32_t const& serial_, xx::ObjBase_s&& o_) {
		switch (o_.typeId()) {
		case xx::TypeId_v<Ping>: {
			auto&& o = o_.ReinterpretCast<Ping>();
			SendResponse<Pong>(serial_, o->ticks);
			break;
		}
		default:
			om.CoutN("ReceiveRequest unhandled package: ", o_);
		}
		om.KillRecursive(o_);
		return 0;
	}
};

struct EchoPeer : xx::PeerCode<EchoPeer>, std::enable_shared_from_this<EchoPeer> {
	EchoPeer(Server& server_, asio::ip::tcp::socket&& socket_) : PeerCode(server_.ioc, std::move(socket_)) {}
	size_t HandleMessage(uint8_t* inBuf, size_t len) {
		Send({ inBuf, len });
		return len;
	}
};

int main() {
	Server server;
	server.Listen<SPeer>(55551);
	std::cout << "simple xx::Data server running... port = 55551" << std::endl;
	server.Listen<EchoPeer>(55555);
	std::cout << "simple echo server running... port = 55555" << std::endl;
	server.ioc.run();
	return 0;
}
