#include "xx_asio.h"
#include "xx_string.h"

struct ABC {
    xx::Asio::Client& c;
    explicit ABC(xx::Asio::Client& c) : c(c) {}
    ABC(ABC&) = delete;
    ABC operator=(ABC&) = delete;

    int r = 0;
    double s = 0;
    int lineNumber = 0;
    int Update() {
        COR_BEGIN;

        // 配置参数
        //c.SetDomainPort("192.168.1.53", 20000);
        c.SetDomainPort("192.168.1.135", 10001);

    LabBegin:
        // 无脑重置一发
        c.Reset();

        // 睡 1 秒，等别的协程事件互动, 避免频繁拨号
        s = xx::NowSteadyEpochSeconds() + 1;
        do {
            COR_YIELD
        }
        while(xx::NowSteadyEpochSeconds() > s);

        // 开始域名解析
        r = c.Resolve();
        if (r) {
            xx::CoutN("c.Resolve() = ", r);
            goto LabBegin;
        }

        // 等解析
        s = xx::NowSteadyEpochSeconds() + 3;
        while(c.Busy()) {
            COR_YIELD
            if (xx::NowSteadyEpochSeconds() > s) {
                xx::CoutN("c.Resolve() Timeout");
                goto LabBegin;
            }
        }

        // 如果解析失败就重试
        if (!c.IsResolved()) goto LabBegin;

        // 成功：打印下 ip 列表
        for (auto& ip : c.GetIPList()) {
            std::cout << ip << std::endl;
        }

        {
            asio::ip::udp::endpoint ep(c.addrs[0], 10001);
            asio::ip::udp::socket socket(c.ioc, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
            socket.send_to(asio::buffer("asdf", 4), ep);

            char udpRecvBuf[1024 * 64];
            asio::error_code e;
            asio::ip::udp::endpoint p;
            auto recvLen = socket.receive_from(asio::buffer(udpRecvBuf), p, 0, e);
            xx::CoutN(recvLen);
        }


        // 开始拨号
        r = c.Dial();
        if (r) {
            xx::CoutN("c.Dial() = ", r);
            goto LabBegin;
        }

        // 等拨号
        s = xx::NowSteadyEpochSeconds() + 5;
        while(c.Busy()) {
            COR_YIELD
            if (xx::NowSteadyEpochSeconds() > s) {
                xx::CoutN("c.Dial() Timeout");
                goto LabBegin;
            }
        }

        // 等 0 号服务
        s = xx::NowSteadyEpochSeconds() + 5;
        while(true) {
            COR_YIELD
            if (!c.Alive()) {
                xx::CoutN("!c.Alive()");
                goto LabBegin;
            }
            if (c.IsOpened(0)) break;
            if (xx::NowSteadyEpochSeconds() > s) {
                xx::CoutN("c.IsOpened(0) Timeout");
                goto LabBegin;
            }
        }

        // todo: 发包

        COR_END
    }
};

int main() {
    xx::Asio::Client c;
    ABC abc(c);
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        c.Update();
        abc.lineNumber = abc.Update();
    }
    while(abc.lineNumber);
	std::cout << "end." << std::endl;
	return 0;
}
