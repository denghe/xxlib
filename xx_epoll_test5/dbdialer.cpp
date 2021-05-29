#include "dbdialer.h"
#include "dbpeer.h"
#include "server.h"
#include "xx_logger.h"

#define S ((Server*)ec)

void DBDialer::Connect(xx::Shared<DBPeer> const &peer) {
    assert(!S->dbPeer);

    // 没连上
    if (!peer) {
        LOG_ERROR("failed");
        return;
    }

    LOG_INFO("success");

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    S->dbPeer = peer;
}
