#include "cpeer.h"
#include "speer.h"
#include "server.h"
#include "config.h"
#include "dialer.h"
#include "xx_logger.h"

bool CPeer::Close(int const& reason, char const* const& desc) {
    // 防重入( 同时关闭 fd )
    if (!this->BaseType::Close(reason, desc)) return false;
    LOG_INFO("CPeer Close. ip = ", addr," reason = ", reason, ", desc = ", desc);
    // 群发断开通知 并从容器移除
    PartialClose();
    // 延迟减持
    DelayUnhold();
    return true;
}

void CPeer::DelayClose(double const& delaySeconds) {
    // 这个的日志记录在调用者那里
    // 避免重复执行
    if (closed || !Alive()) return;
    // 标记为延迟自杀
    closed = true;
    // 利用超时来 Close
    SetTimeoutSeconds(delaySeconds <= 0 ? 3 : delaySeconds);
    // 群发断开指令 并从容器移除
    PartialClose();
}

void CPeer::PartialClose() {
    // 群发断开通知
    for (auto &&sid : serverIds) {
        if (auto&& sp = ((Server *)ec)->dps[sid].second) {
            sp->SendCommand("disconnect", clientId);
        }
    }
    // 从容器移除( 减持 )
    ((Server *)ec)->cps.erase(clientId);
}

void CPeer::ReceivePackage(uint8_t *const &buf, size_t const &len) {
    // 取出 serverId
    auto sid = *(uint32_t *) buf;

    // 判断该服务编号是否在白名单中. 找不到就忽略
    if (serverIds.find(sid) == serverIds.end()) {
        LOG_INFO("CPeer ReceivePackage serverIds.find(sid) == serverIds.end(). ip = ", addr, ", serverId = ", sid, ", serverIds = ", serverIds);
        return;
    }

    // 指向目标服务 peer
    auto&& sp = ((Server *)ec)->dps[sid].second;
    // 如果服务 peer 当前无效，则忽略
    if (!sp) {
        LOG_INFO("CPeer ReceivePackage !sp || !sp->Alive(). ip = ", addr, ", serverId = ", sid, ", serverIds = ", serverIds);
        return;
    }

    LOG_INFO("CPeer ReceivePackage. ip = ", addr, ", serverId = ", sid, ", buf len = ", len);

    // 续命. 每次收到合法数据续一下
    SetTimeoutSeconds(config.clientTimeoutSeconds);

    // 篡改 serverId 为 clientId
    *(uint32_t *) buf = clientId;

    // 用 serverId 对应的 peer 转发完整数据包
    sp->Send({buf - 4, len + 4});
}

void CPeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    LOG_INFO("CPeer ReceiveCommand. ip = ", addr, ", len = ", len, ", buf = ", xx::DataView{ buf, len });

    // 续命
    SetTimeoutSeconds(config.clientTimeoutSeconds);

    // echo 发回( buf 指向了 header + 0xffffffff 之后的区域，故 -8 指向 header )
    Send(buf - 8, len + 8);
    Flush();
}
