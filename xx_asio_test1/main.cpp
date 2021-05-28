#include "xx_asio.h"
#include "xx_string.h"
#include "xx_coro.h"

struct Client : xx::Asio::Client {
    xx::Coros coros;
    Client() : coros(100, 100 * 60 * 5) {
        coros.Add(Logic());
    }
    void FrameUpdate() {
        Update();
        coros();
    }

    bool recvEcho = false;
    double ping = 0;
    xx::Data d;

    xx::Coro Logic() {
        // init by config
        SetDomainPort("192.168.1.135", 10001);

    LabBegin:
        // 无脑重置一发
        Reset();

        // 睡 1 秒，等别的协程事件互动, 避免频繁拨号
        co_yield 1;

        // 开始域名解析
        if (int r = Resolve()) {
            xx::CoutN("c.Resolve() = ", r);
            goto LabBegin;
        }

        // 等解析 3 秒
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });
        if (!IsResolved()) goto LabBegin;

        // 打印下 ip 列表
        for (auto& ip : GetIPList()) {
            std::cout << ip << std::endl;
        }

        // 开始拨号
        if (int r = Dial()) {
            xx::CoutN("c.Dial() = ", r);
            goto LabBegin;
        }

        // 等拨号 3 秒
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });
        if (Busy()) goto LabBegin;

        // 等 0 号服务 5 秒
        co_yield xx::Cond(5).Update([&]{ return Alive() ? IsOpened(0) : true; });
        if (!Alive() || coros.isTimeout) goto LabBegin;

        // 设置收到 echo 包的回调: 算 ping 并清除 flag
        peer->onReceiveEcho = [this](xx::Data const& d)->int {
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
        peer->SendEcho(d);

        // 10 秒没收到 echo 回包就掐
        co_yield xx::Cond(10).Update([&]{ return Alive() ? recvEcho : true; });
        if (!Alive() || coros.isTimeout) goto LabBegin;

        xx::CoutN("ping = ", ping);

        // 等 1 秒
        co_yield 1;
        if (!Alive()) goto LabBegin;
        goto LabKeepAlive;
    }
};

int main() {
    Client c;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100 fps
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
    }
    while(c.coros);
	std::cout << "end." << std::endl;
	return 0;
}
