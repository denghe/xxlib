#include "timer.h"
#include "server.h"
#include "dbdialer.h"
#include "dbpeer.h"
#include "ldialer.h"
#include "lpeer.h"

#define S ((Server*)ec)

void Timer::Start() {
    SetTimeoutSeconds(intervalSeconds);
}

void Timer::Timeout() {
    auto &&now = xx::NowSteadyEpochMilliseconds();

    // dial to db
    if (!S->dbPeer) {
        auto& d = S->dbDialer;
        if (!d->Busy()) {
            d->DialSeconds(2);
        }
    }

    // dial to lobby
    if (!S->lobbyPeer) {
        auto& d = S->lobbyDialer;
        if (!d->Busy()) {
            d->DialSeconds(2);
        }
    }

    // timer 续命
    SetTimeoutSeconds(intervalSeconds);
}
