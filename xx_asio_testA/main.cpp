// lobby + gateway + client -- lobby
#include "../xx_asio_test8/xx_asio_codes.h"
#include "pkg.h"

struct Server : xx::ServerCode<Server> {
	xx::ObjManager om;
	xx::Shared<Pong> pkg_pong = xx::Make<Pong>();
};

struct GPeer : xx::PeerCode<GPeer>, xx::PeerTimeoutCode<GPeer>, xx::PeerRequestCode<GPeer, true>, std::enable_shared_from_this<GPeer> {
	Server& server;
	GPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{}
	int ReceiveTargetRequest(uint32_t target, int32_t serial_, xx::ObjBase_s&& o_) {
		switch (o_.typeId()) {
		case xx::TypeId_v<Ping>: {
			auto&& o = o_.ReinterpretCast<Ping>();
			SendResponse<Pong>(0, serial_, o->ticks);
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
	server.Listen<GPeer>(54321);
	std::cout << "lobby + gateway + client -- lobby running... port = 54321" << std::endl;
	server.ioc.run();
	return 0;
}
