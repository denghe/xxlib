#include "dialer.h"
#include "server.h"
#include "config.h"
#include "xx_logger.h"

Server &Dialer::GetServer() {
    // 拿到服务上下文
    return *(Server *) &*ec;
}

void Dialer::Connect(xx::Shared<SPeer> const &peer) {
    // 没连上
    if (!peer) {
        LOG_INFO("Dialer Connect failed. serverId = ", serverId);
        return;
    }

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    GetServer().dps[serverId].second = peer;

    // 继续填充一些依赖
    peer->serverId = serverId;

    // 向 server 发送自己的 gatewayId
    peer->SendCommand("gatewayId", config.gatewayId);

    LOG_INFO("Dialer Connect. serverId = ", serverId);
}
