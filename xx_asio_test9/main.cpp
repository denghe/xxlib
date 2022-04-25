// asio simple tcp client( xx::Data container )
#include "../xx_asio_test8/xx_asio_codes.h"
#include <pkg_generic.h>

struct Client : xx::ServerCode<Client> {
	xx::ObjManager om;
	void Run(uint16_t const& port);
};

struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer>, std::enable_shared_from_this<CPeer> {
	Client& client;
	CPeer(Client& client_)
		: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
		, PeerTimeoutCode(client_.ioc)
		, PeerRequestCode(client_.om)
		, client(client_)
	{}

	void Start_() {
		asio::co_spawn(ioc, [this, self = shared_from_this()]()->asio::awaitable<void> {
			//co_await 
			//SendRequest()
		}, asio::detached);
	}
};

void Client::Run(uint16_t const& port) {

}

int main() {
	Client c;
	uint16_t port = 55551;
	std::cout << " asio simple tcp client( xx::Data container ) running... port = " << port << std::endl;
	c.Run(port);
	return 0;
}
