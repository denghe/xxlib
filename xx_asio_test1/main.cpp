#include "xx_asio.h"
#include "xx_string.h"

struct ABC {
    xx::Asio::Client& c;
    explicit ABC(xx::Asio::Client& c) : c(c) {}
    ABC(ABC&) = delete;
    ABC operator=(ABC&) = delete;

    bool recvEcho = false;
    int r = 0;
    double s = 0;
    double ping = 0;
    xx::Data d;

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
        while(xx::NowSteadyEpochSeconds() < s);

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

        // 设置收到 echo 包的回调: 算 ping 并清除 flag
        c.peer->onReceiveEcho = [this](xx::Data const& d)->int {
            //xx::CoutN("reccv echo: ", d);
            assert(d.len == 8);
            xx::Data_r dr(d.buf, d.len);
            double v;
            if (int rtv = dr.ReadFixed(v)) return rtv;
            ping = xx::NowSteadyEpochSeconds() - v;
            recvEcho = true;
            return 0;
        };

        // 简单实现一个 ping 逻辑. loop: 发送当前 secs, 在收到回包后间隔 1 秒再发
    LabKeepAlive:
        recvEcho = false;
        d.Clear();
        d.WriteFixed(xx::NowSteadyEpochSeconds());
        c.peer->SendEcho(d);

        // 10 秒没收到 echo 回包就掐
        s = xx::NowSteadyEpochSeconds() + 10;
        do {
            COR_YIELD
            if (!c.Alive()) {
                xx::CoutN("!c.Alive()");
                goto LabBegin;
            }
            if (recvEcho) break;
        }
        while(xx::NowSteadyEpochSeconds() < s);

        if (!recvEcho) {
            xx::CoutN("recv ping timeout");
            goto LabBegin;
        }
        else {
            xx::CoutN("ping = ", ping);
        }

        // 等 1 秒
        s = xx::NowSteadyEpochSeconds() + 1;
        do {
            COR_YIELD
            if (!c.Alive()) {
                xx::CoutN("!c.Alive()");
                goto LabBegin;
            }
        }
        while(xx::NowSteadyEpochSeconds() < s);
        goto LabKeepAlive;

        COR_END
    }
};

int main() {
    xx::Asio::Client c;

    ABC abc(c);
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        c.Update();
        {
            // 模拟 lua 收包
            xx::Asio::Package p;
            (void)c.TryGetPackage(p);
        }
        {
            // 产生 C++ 回调
            if (c.peer) c.peer->HandleReceivedCppPackages();
        }
        abc.lineNumber = abc.Update();
    }
    while(abc.lineNumber);
	std::cout << "end." << std::endl;
	return 0;
}
