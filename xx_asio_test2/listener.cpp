#include "listener.h"
#include "server.h"
#include "peer.h"

void Listener::Accept(xx::Shared<Peer> const &p) {
    // 没连上
    if (!p) return;

    // call game logic
    if (!((Server*)ec)->Accept(p)) {
        p->Hold();
    }
}
