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

        // 配置参数
        c.SetDomainPort("www.baidu.com", 10000);

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
        c.Resolve();
        while(c.Busy()) {
            COR_YIELD
        }

        // 如果解析失败就重试
        if (c.IPListIsEmpty()) goto LabBegin;

        // 成功：打印下 ip 列表
        for (auto& ip : c.GetIPList()) {
            std::cout << ip << std::endl;
        }

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
