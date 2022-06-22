// asio simple tcp server( xx::Data container )
#include "xx_asio_codes.h"

struct Server : xx::IOCCode<Server> {
};

struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, xx::PeerRequestDataCode<SPeer>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{}
	int ReceiveRequest(int32_t const& serial_, xx::Data_r& dr) {
		uint16_t typeId;
		if (int r = dr.ReadVarInteger(typeId)) return r;
		switch (typeId) {
		case 1: {
			std::string s;
			if (int r = dr.Read(s)) return r;
			xx::CoutN("ReceiveRequest Data: typeId = ", typeId, " s = ", s);
			SendResponse(serial_, [&](xx::Data& d) {
				d.WriteVarInteger(uint16_t(2));
				d.Write("ok...");
			});
			break;
		}
		default:
			xx::CoutN("ReceiveRequest unhandled package: ", dr);
		}
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
