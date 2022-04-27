// lobby + gateway + client -- gateway
#include "../xx_asio_test8/xx_asio_codes.h"
struct Server : xx::ServerCode<Server> {
};

struct SPeer : xx::PeerCode<SPeer>, xx::PeerTimeoutCode<SPeer>, std::enable_shared_from_this<SPeer> {
	Server& server;
	SPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{}
};

int main() {
	Server server;
	server.Listen<SPeer>(54322);
	std::cout << "lobby + gateway + client -- gateway running... port = 54322" << std::endl;
	server.ioc.run();
	return 0;
}
