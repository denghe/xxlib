// lobby + gateway + client -- lobby
#include "../xx_asio_test8/xx_asio_codes.h"
#include "pkg.h"

struct Server : xx::ServerCode<Server> {
	xx::ObjManager om;
	xx::Shared<Pong> pkg_pong = xx::Make<Pong>();
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
			server.pkg_pong->ticks = o->ticks;
			SendResponse(serial_, server.pkg_pong);
			break;
		}
		default:
			om.CoutN("ReceiveRequest unhandled package: ", o_);
		}
		om.KillRecursive(o_);
		return 0;
	}
};

int main() {
	Server server;
	server.Listen<SPeer>(54321);
	std::cout << "lobby + gateway + client -- lobby running... port = 54321" << std::endl;
	server.ioc.run();
	return 0;
}
