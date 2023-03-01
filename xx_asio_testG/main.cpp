#include <xx_asio_gateway_client.h>

int main() {

	xx::Asio::Gateway::Client c;

	co_spawn(c, [&]()->awaitable<void> {

		c.SetDomainPort("127.0.0.1", 20000);
		c.SetCppServerId(0);

	LabBegin:
		std::cout << "********** LabBegin ************" << std::endl;

		std::cout << "c.Dial()" << std::endl;
		if (auto r = co_await c.Dial()) {
			std::cout << "c.Dail() failed. r = " << " r = " << r << std::endl;
			co_await xx::Timeout(1000ms);
			goto LabBegin;
		}

		std::cout << "c.WaitOpens(5s, 0)" << std::endl;
		if (!co_await c.WaitOpens(5s, 0)) {
			std::cout << "c.WaitOpens(5s, 0) timeout" << std::endl;
			co_await xx::Timeout(1000ms);
			goto LabBegin;
		}

		std::cout << "todo c.Send ?????" << std::endl;
		while (true) {
			co_await xx::Timeout(10ms);
			std::cout << ".";
			std::cout.flush();
		}

	}, detached);

	while (true) {
		c.Update();
		Sleep(16);
		c.Update();
	}

	return 0;
}
