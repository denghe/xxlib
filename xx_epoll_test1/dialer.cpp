#include "dialer.h"
#include "server.h"
#include "config.h"
#include "xx_logger.h"

void Dialer::Connect(xx::Shared<SPeer> const &peer) {
    // 没连上
    if (!peer) {
        LOG_ERROR("failed. serverId = ", serverId);
        return;
    }

    LOG_INFO("success. serverId = ", serverId);

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    ((Server *)ec)->dps[serverId].second = peer;

    // 继续填充一些依赖
    peer->serverId = serverId;

    // 向 server 发送自己的 gatewayId
    peer->SendCommand("gatewayId", config.gatewayId);
}
