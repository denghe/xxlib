#include "ldialer.h"
#include "lpeer.h"
#include "server.h"
#include "config.h"
#include "xx_logger.h"

void LDialer::Connect(xx::Shared<LPeer> const &peer) {
    // 没连上
    if (!peer) {
        LOG_INFO("failed");
        return;
    }

    LOG_INFO("success");

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    ((Server *)ec)->lobbyPeer = peer;
}
