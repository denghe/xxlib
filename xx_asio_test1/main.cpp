#include "xx_asio.h"
#include "xx_string.h"
#include "xx_coro.h"
#include "pkg_lobby_client.h"
#include "pkg_game_client.h"

#define CLIENT_FPS 60

struct Client : xx::Asio::Client {
    xx::Coros coros;

    Client() : coros(CLIENT_FPS, CLIENT_FPS * 60 * 5) {
        coros.Add(Coro_Dial_Auth());
        coros.Add(Coro_Ping());
    }

    void Update2() {
        // 驱动 C++ 回调
        if (peer) peer->HandleReceivedCppPackages();
        coros();
    }

    bool recvEcho = false;
    double ping = 0;

    xx::Coro Coro_Dial_Auth() {
        // init by config
        SetDomainPort("192.168.1.135", 10001);

    LabBegin:
        xx::CoutTN("LabBegin");
        // 无脑重置一发
        Reset();

        // 睡 1 秒，避免频繁拨号
        co_yield 1;

        // 开始域名解析
        if (int r = Resolve()) {
            xx::CoutTN("c.Resolve() = ", r);
            goto LabBegin;
        }

        // 等 3 秒, 如果解析完成就停止等待
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });

        // 解析失败: 重来
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

        // 等 3 秒, 如果拨号完成就停止等待
        co_yield xx::Cond(3).Update([&]{ return !Busy(); });

        // 没连上: 重来
        if (!Alive()) goto LabBegin;

        // 等 5 秒，如果断线或 0 号服务的 open 指令到达，就停止等待
        co_yield xx::Cond(5).Update([&]{ return Alive() ? IsOpened(0) : true; });

        // 断线或超时: 重来
        if (!Alive() || coros.isTimeout) goto LabBegin;

        // 设置 cpp 接管 服务id, 同时也是发送目的地
        peer->SetCppServiceId(0);

        // Request 结果容器
        xx::ObjBase_s rtv;

        //// 发 Auth 校验包
        //SendRequest<Client_Lobby::Auth>(rtv, "abc", "123");

        //// 等 15 秒，如果收到 回包 就停止等待
        //co_yield xx::Cond(15).Update([&] { return Alive() ? rtv : true; });

        co_yield xx::Cond(15).Event(SendRequest<Client_Lobby::Auth>(rtv, "abc", "123"));

        // 结果空: 重来
        if (!rtv) goto LabBegin;

        // 输出
        peer->om.CoutN(rtv);

        // try again
        goto LabBegin;
    }

    // 发请求. 结果保存到 rtv
    template<typename PKG = xx::ObjBase, typename ... Args>
    int SendRequest(xx::ObjBase_s& rtv, Args const &... args) {
        return peer->SendRequest<PKG>([this, &rtv](int32_t const& serial_, xx::ObjBase_s&& ob) {
            rtv = std::move(ob);
            coros.FireEvent(serial_);
        }, 99999.0, args...);
    }

    xx::Coro Coro_Ping() {
        // 一直等待直到 0 号服务就绪
    LabBegin:
        co_yield 0;
        if (!IsOpened(0)) goto LabBegin;

        // 设置收到 echo 包的回调
        peer->onReceiveEcho = [this](auto&& a1) { return CalcPing(std::forward<decltype(a1)>(a1)); };

        // ping 包容器
        xx::Data d;

        // 简单实现一个 ping 逻辑. while(true) { 发送当前 secs, 等回包算ping, 等1秒 }
        while (true) {
            recvEcho = false;

            // 构造 ping 包
            d.Clear();
            d.WriteFixed(xx::NowSteadyEpochSeconds());

            // 以 echo 指令方式直发网关
            peer->SendEcho(d);

            // 断线或 超时没收到 echo 回包就退出等待
            co_yield xx::Cond(10).Update([&] { return Alive() ? recvEcho : true; });

            // 断线：回到开始去等待
            if (!Alive()) goto LabBegin;

            // 超时：强制断线，并回到开始去等待
            if (coros.isTimeout) {
                Reset();
                goto LabBegin;
            }

            // 等到 ping 包，打印( onReceiveEcho 里算的 )
            xx::CoutN("ping = ", ping);

            // 等 1 秒, 如果断线就停止等待
            co_yield xx::Cond(1).Update([&] { return !Alive(); });

            // 断线：回到开始去等待
            if (!Alive()) goto LabBegin;
        }
    }

    // 算 ping 并清除 flag
    int CalcPing(xx::Data const& d_) {
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
