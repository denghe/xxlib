#include "timer.h"
#include "server.h"
#include "dbdialer.h"
#include "dbpeer.h"

#define S ((Server*)ec)

void Timer::Start() {
    SetTimeoutSeconds(intervalSeconds);
}

void Timer::Timeout() {
    auto &&now = xx::NowSteadyEpochMilliseconds();

//    // 遍历当前已存在的服务器间连接容器
//    for (auto &&kv : S->sps) {
//        // 如果连接存在
//        if (auto &&sp = dp.second) {
//            // 如果正在等
//            if (sp->waitingPingBack) {
//                // 如果已经等了好些时候了( 该值可配？）
//                if(now - sp->lastSendPingMS > 5000) {
//                    // 掐线
//                    sp->Close(-__LINE__, " PingTimer Timeout if(now - sp->lastSendPingMS > 5000)");
//                }
//            }
//            else {
//                // 发起 ping
//                sp->lastSendPingMS = now;
//                sp->SendCommand("ping", now);
//                sp->waitingPingBack = true;
//            }
//        }
//    }

    // dial to db
    if (!S->dbPeer) {
        auto& d = S->dbDialer;
        if (!d->Busy()) {
            d->DialSeconds(2);
        }
    }

    // timer 续命
    SetTimeoutSeconds(intervalSeconds);
}
