// asio simple tcp server( xx::Data container )
#include "xx_asio_codes.h"
#include <xx_obj.h>

struct Server : xx::ServerCode<Server> {
	xx::ObjManager om;
	Server() {
		//om.Register()
	}
};

struct SPeer : xx::PeerCode<SPeer, true, true>, xx::PeerTimeoutCode<SPeer>, xx::PeerRequestCode<SPeer>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{}

	void HandleMessage(xx::Data& msg) {
		if (stoping) {
			msg.Clear();
			return;
		}

		// todo: unpack & handle

		Send({ msg.buf, msg.len });	// echo
		msg.offset = msg.len;		// 模拟读出数据

		msg.RemoveFront(msg.offset);
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
