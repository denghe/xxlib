#include "listener.h"
#include "server.h"
#include "peer.h"
#include "xx_logger.h"

void Listener::Accept(xx::Shared<Peer> const &p) {
    // 没连上
    if (!p) return;

    // 引用到 server 备用
    auto &&s = *(Server*)ec;

    // 加持
    p->Hold();

    // 填充自增id
    p->clientId = ++s.pid;

    // 放入容器
    s.ps[p->clientId] = p;

    // 设置自动断线超时时间
    p->SetTimeoutSeconds(15);

    // todo: game logic here

    LOG_INFO("success. ip = ", p->addr, ", clientId = ", p->clientId);
}
