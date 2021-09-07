#include "xx_asiokcpclient.h"

struct Logic {
    xx::AsioKcpClient c;

    double secs = 0;

    int lineNumber = 0;

    int Update() {
        c.Update();

        COR_BEGIN
                // 初始化拨号地址
                c.SetDomainPort("192.168.1.95", 12345);

            LabBegin:
                xx::CoutTN("LabBegin");
                // 无脑重置一发
                c.Reset();

                // 睡一秒
                secs = xx::NowEpochSeconds() + 1;
                do {
                    COR_YIELD;
                } while (secs > xx::NowEpochSeconds());

                // 开始域名解析
                xx::CoutTN("Resolve");
                c.Resolve();

                // 等 3 秒, 如果解析完成就停止等待
                secs = xx::NowEpochSeconds() + 3;
                do {
                    COR_YIELD;
                    if (!c.Busy()) break;
                } while (secs > xx::NowEpochSeconds());

                // 解析失败: 重来
                if (!c.IsResolved()) goto LabBegin;

                // 打印下 ip 列表
                for (auto& ip : c.GetIPList()) {
                    xx::CoutTN(ip);
                }

                // 开始拨号
                if (int r = c.Dial()) {
                    xx::CoutTN("c.Dial() = ", r);
                    goto LabBegin;
                }
                xx::CoutTN("Dial");

                // 等 3 秒, 如果拨号完成就停止等待
                secs = xx::NowEpochSeconds() + 3;
                do {
                    COR_YIELD;
                    if (!c.Busy()) break;
                } while (secs > xx::NowEpochSeconds());

                // 没连上: 重来
                if (!c.Alive()) goto LabBegin;

                xx::CoutTN("Send");
                // 发点啥？
                //c.Send(o);
                c.peer->Send((uint8_t*)"abc", 3);

                xx::CoutTN("keep alive");
                // 如果断线就重连
                do {
                    COR_YIELD;
                } while (c.Alive());
                goto LabBegin;
        COR_END
    }
};

int main() {
    Logic c;
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
    } while(c.lineNumber = c.Update());
    std::cout << "end." << std::endl;
    return 0;
}
