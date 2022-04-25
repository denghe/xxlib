// asio simple tcp client( xx::Data container )
#include "../xx_asio_test8/xx_asio_codes.h"

struct CPeer : xx::PeerCode<CPeer>, std::enable_shared_from_this<CPeer> {
	asio::io_context& ioc;
	asio::steady_timer recvBlocker;
	std::vector<std::string> recvs;

	CPeer(asio::io_context& ioc_)
		: PeerCode(ioc_, asio::ip::tcp::socket(ioc_))
		, ioc(ioc_)
		, recvBlocker(ioc_, std::chrono::steady_clock::time_point::max())
	{
	}

	void StartAfter() {}

	void StopAfter() {
		recvBlocker.cancel();
	}

	void HandleMessage(std::string_view const& msg) {
		recvs.emplace_back(std::string(msg));
		recvBlocker.cancel_one();
	}
};

int main() {
	//MyServer server;
	//uint16_t port = 55551;
	//server.Listen<MyPeer>(port);
	//std::cout << "simple telnet server running... port = " << port << std::endl;
	//server.ioc.run();
	return 0;
}
