// asio simple tcp server( xx::Data container )
#include "xx_asio_codes.h"
#include <pkg_generic.h>

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
		case xx::TypeId_v<Generic::Error>: {
			auto&& o = o_.ReinterpretCast<Generic::Error>();
			o->errorCode++;
			o->errorMessage += "!!!";
			SendResponse(serial_, o);
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
	uint16_t port = 55551;
	server.Listen<SPeer>(port);
	std::cout << "simple xx::Data server running... port = " << port << std::endl;
	server.ioc.run();
	return 0;
}
