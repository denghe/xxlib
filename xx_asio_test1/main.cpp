#include "xx_asio.h"
#include "xx_string.h"
#include "xx_coro.h"

#define CLIENT_FPS 60

struct Client : xx::Asio::Client {
    xx::Coros coros;
    Client() : coros(CLIENT_FPS, CLIENT_FPS * 60 * 5) {
        coros.Add(Logic());
    }
    void Update2() {
        // 产生 C++ 回调
        if (peer) peer->HandleReceivedCppPackages();
        coros();
    }

    bool recvEcho = false;
    double ping = 0;
    xx::Data d;

    xx::Coro Logic() {
        // init by config
        SetDomainPort("192.168.1.135", 10001);

    LabBegin:
        xx::CoutTN("LabBegin");
        // 无脑重置一发
        Reset();

        // 睡 1 秒，等别的协程事件互动, 避免频繁拨号
        co_yield 1;

        // 开始域名解析
        if (int r = Resolve()) {
            xx::CoutTN("c.Resolve() = ", r);
            goto LabBegin;
        }

        // 等解析 3 秒
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });
        if (!IsResolved()) goto LabBegin;

        // 打印下 ip 列表
        for (auto& ip : GetIPList()) {
            xx::CoutTN(ip);
        }

        // 开始拨号
        if (int r = Dial()) {
            xx::CoutTN("c.Dial() = ", r);
            goto LabBegin;
        }

        // 等拨号 3 秒
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });
        if (Busy()) goto LabBegin;

        // 等 0 号服务 5 秒
        co_yield xx::Cond(5).Update([&]{ return Alive() ? IsOpened(0) : true; });
        if (!Alive() || coros.isTimeout) goto LabBegin;

        // 设置收到 echo 包的回调
        peer->onReceiveEcho = [this](auto && a1) { return HandlePing(std::forward<decltype(a1)>(a1)); };

        // 简单实现一个 ping 逻辑. while(true) { 发送当前 secs, 等回包算ping, 等1秒 }
    LabKeepAlive:
        recvEcho = false;
        d.Clear();
        d.WriteFixed(xx::NowSteadyEpochSeconds());
        peer->SendEcho(d);

        // 10 秒没收到 echo 回包就掐
        co_yield xx::Cond(10).Update([&]{ return Alive() ? recvEcho : true; });
        if (!Alive() || coros.isTimeout) goto LabBegin;

        // ping 是在 onReceiveEcho 里算的
        xx::CoutN("ping = ", ping);

        // 等 1 秒
        co_yield 1;
        if (!Alive()) goto LabBegin;
        goto LabKeepAlive;
    }

    // 算 ping 并清除 flag
    int HandlePing(xx::Data const& d_) {
        //xx::CoutN("reccv echo: ", d);
        assert(d_.len == 8);
        xx::Data_r dr(d_.buf, d_.len);
        double v;
        if (int rtv = dr.ReadFixed(v)) return rtv;
        ping = xx::NowSteadyEpochSeconds() - v;
        recvEcho = true;
        return 0;
    };
};

int main() {
    Client c;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / CLIENT_FPS));
        // 驱动 asio
        c.Update();
        {
            // 模拟 lua 收包
            xx::Asio::Package p;
            (void)c.TryGetPackage(p);
        }
        // 驱动 c++ 回调 和 协程
        c.Update2();
    }
    while(c.coros);
	std::cout << "end." << std::endl;
	return 0;
}
