#include "tasktimer.h"
#include "server.h"
#include "dialer.h"

void TaskTimer::Start() {
    SetTimeoutSeconds(intervalSeconds);
}

void TaskTimer::Timeout() {
    auto &&server = *(Server *) &*ec;
    // 自动拨号 & 重连逻辑
    for (auto &&iter : server.dps) {
        auto &&dialer = iter.second.first;
        auto &&peer = iter.second.second;
        // 未建立连接
        if (!peer) {
            // 并非正在拨号
            if (!dialer->Busy()) {
                // 超时时间 2 秒
                dialer->DialSeconds(2);
            }
        }
    }

    // timer 续命
    SetTimeoutSeconds(intervalSeconds);
}
