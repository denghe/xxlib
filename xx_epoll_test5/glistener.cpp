#include "glistener.h"
#include "gpeer.h"
#include "config.h"
#include "xx_logger.h"

void GListener::Accept(xx::Shared<GPeer> const &p) {
    // 没连上
    if (!p) return;

    // 加持
    p->Hold();

    // 设置自动断线超时时间
    p->SetTimeoutSeconds(config.peerTimeoutSeconds);

    LOG_INFO("ip = ", p->addr);
}
