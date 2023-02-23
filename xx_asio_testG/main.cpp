#include <xx_asio_kcp_gateway_client.h>

int main() {
	//IOC ioc;
	//co_spawn(ioc, [&]()->awaitable<void> {
	//	auto kp = xx::Make<xx::Asio::Kcp::Gateway::KcpClientPeer>(ioc);
	//	kp->SetServerIPPort("127.0.0.1", 20000);
	//	auto r = co_await kp->Connect();
	//	std::cout << " r = " << r << " ";
	//	while (true) {
	//		co_await xx::Timeout(10ms);
	//		std::cout << ".";
	//		std::cout.flush();
	//	}
	//}, detached);
	//return (int)ioc.run();
	return 0;
}
