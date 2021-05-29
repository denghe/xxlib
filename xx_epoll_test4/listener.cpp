#include "listener.h"
#include "peer.h"
#include "xx_logger.h"

void Listener::Accept(xx::Shared<Peer> const &p) {
    // 没连上
    if (!p) return;

    // 加持
    p->Hold();

    LOG_INFO("ip = ", p->addr);
}
