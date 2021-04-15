#include "xx_asio.h"
#include "xx_string.h"

struct ABC {
    xx::Asio::Client& c;
    explicit ABC(xx::Asio::Client& c) : c(c) {}
    ABC(ABC&) = delete;
    ABC operator=(ABC&) = delete;

    double s = 0;
    int lineNumber = 0;
    int Update() {
        COR_BEGIN;
    LabBegin:
        //c.Cancel();

        s = xx::NowSteadyEpochSeconds() + 1;
        do {
            COR_YIELD
        }
        while(xx::NowSteadyEpochSeconds() > s);

        c.Resolve("www.baidu.com");
        while(c.Busy()) {
            COR_YIELD
        }

        if (c.addrs.empty()) goto LabBegin;

        {
            auto ips = c.GetIPList();
            for (auto& ip : ips) {
                std::cout << ip << std::endl;
            }
        }

        c.SetPort(10000);

        // todo Dial

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
