// asio simple tcp client( xx::Data container )
#include "xx_asio_codes.h"
#include <pkg.h>

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
};

void Client::Run(uint16_t const& port) {
	// 模拟一个客户端连上来
	co_spawn(ioc, [this, port]()->awaitable<void> {
		auto d = std::make_shared<CPeer>(*this);

		// 如果没连上，就反复的连     // todo:退出机制?
		while (d->stoped) {
			auto r = co_await d->Connect(asio::ip::address::from_string("127.0.0.1"), port);
			std::cout << "client Connect r = " << r << std::endl;
		}
		om.CoutN("d->stoped = ", d->stoped);

		// 连上了，发 Ping
		if (auto o = co_await d->SendRequest<Ping>(15s, xx::NowEpoch10m()); !o) {
			om.CoutN("SendRequest timeout!");
		}
		else {
			om.CoutN("receive o = ", o);
			om.KillRecursive(o);
		}

		om.CoutN("d->stoped = ", d->stoped);

		// 等一段时间退出
		co_await xx::Timeout(60s);
	}, detached);
	ioc.run();
}

int main() {
	Client c;
	uint16_t port = 55551;
	std::cout << " asio simple tcp client( xx::Data container ) running... port = " << port << std::endl;
	c.Run(port);
	return 0;
}
