#include "dbdialer.h"
#include "dbpeer.h"
#include "server.h"
#include "config.h"
#include "xx_logger.h"

void DBDialer::Connect(xx::Shared<DBPeer> const &peer) {
    // 没连上
    if (!peer) {
        LOG_INFO("failed");
        return;
    }

    LOG_INFO("success");

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    ((Server *)ec)->dbPeer = peer;
}
