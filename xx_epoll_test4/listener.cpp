#include "listener.h"
#include "peer.h"
#include "xx_logger.h"

#define S ((Server*)ec)

void Listener::Accept(xx::Shared<Peer> const &p) {
    // 没连上
    if (!p) return;

    // 加持
    p->Hold();

    // store
    S->peers[p.pointer] = p;

    LOG_INFO("ip = ", p->addr);
}
