#include "speer.h"
#include "server.h"
#include "cpeer.h"
#include "dialer.h"
#include "xx_logger.h"

bool SPeer::Close(int const& reason, char const* const& desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Peer::Close(reason, desc)) return false;
    LOG_INFO("SPeer Close. serverId = ", serverId, ", reason = ", reason, ", desc = ", desc);
    // 从所有 client peers 里的白名单中移除
    for (auto &&kv : ((Server*)ec)->cps) {
        // 不管有没有, 试着 从 client peer 的 已open id列表 擦除该 服务id
        kv.second->serverIds.erase(serverId);
        // 下发 close( 如果全都没了, 则会导致 client 无法发送任何数据过来，自然超时断掉 )
        kv.second->SendCommand("close", serverId);
        kv.second->Flush();
    }
    // 从 dps 移除以减持
    ((Server*)ec)->dps[serverId].second.Reset();
    // 延迟减持
    DelayUnhold();
    return true;
}

void SPeer::ReceivePackage(uint8_t *const &buf, size_t const &len) {
    // 读出 clientId
    auto&& clientId = *(uint32_t *) buf;
    // 如果找到 client peer, 则转发
    if(auto &&cp = TryGetCPeer(clientId)) {
        LOG_INFO("SPeer ReceivePackage. serverId = ", serverId, ", clientId = ", clientId, ", buf len = ", len);
        // 篡改 clientId 为 serverId
        *(uint32_t *) buf = serverId;

        // 从 长度包头 处开始，直接转发
        cp->Send(buf - 4, len + 4);
    }
    else {
        // 没找到：输出点日志
        LOG_INFO("SPeer ReceivePackage TryGetCPeer failed. serverId = ", serverId, ", clientId = ", clientId, ", buf len = ", len);
    }
}

void SPeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    // 创建一个数据读取器
    xx::Data_r dr(buf, len);

    // 试读取 cmd 字串. 失败直接断开
    std::string cmd;
    if (int r = dr.Read(cmd)) {
        Close(-30, xx::ToString(__LINE__, " SPeer ReceiveCommand if (int r = dr.Read(cmd)) serverId = ", serverId, ", r = ", r).c_str());
        return;
    }

    // 公用 client id 容器
    uint32_t clientId = 0;

    if (cmd == "open") {                        // 向客户端开放 serverId. 参数: clientId
        // 试读出 clientId. 失败直接断开
        if (int r = dr.Read(clientId)) {
            Close(-31, xx::ToString(__LINE__, " SPeer ReceiveCommand if (int r = dr.Read(clientId)) serverId = ", serverId, ", r = ", r).c_str());
            return;
        }
        // 如果找到 client peer, 则转发
        if (auto &&cp = TryGetCPeer(clientId)) {
            LOG_INFO("SPeer ReceiveCommand 'open'. serverId = ", serverId, ", clientId = ", clientId);
            // 放入白名单
            cp->serverIds.emplace(serverId);
            // 下发 open
            cp->SendCommand("open", serverId);
            cp->Flush();
        }
        else {
            // 未找到
            LOG_INFO("SPeer ReceiveCommand 'open'. TryGetCPeer failed. serverId = ", serverId, ", clientId = ", clientId);
        }
    } else if (cmd == "close") {                // 关端口. 参数: clientId
        // 试读出 clientId. 失败直接断开
        if (int r = dr.Read(clientId)) {
            Close(-32, xx::ToString(__LINE__, " SPeer ReceiveCommand Read 'close' failed. serverId = ", serverId, ", r = ", r).c_str());
            return;
        }
        // 如果找到 client peer, 则转发
        if (auto &&cp = TryGetCPeer(clientId)) {
            LOG_INFO("SPeer ReceiveCommand 'close'. serverId = ", serverId, ", clientId = ", clientId);
            // 从白名单移除
            cp->serverIds.erase(serverId);
            // 下发 close
            cp->SendCommand("close", serverId);
            cp->Flush();
        }
        else {
            // 未找到
            LOG_INFO("SPeer ReceiveCommand 'close'. TryGetCPeer failed. serverId = ", serverId, ", clientId = ", clientId);
        }
    } else if (cmd == "kick") {                 // 踢玩家下线. 参数: clientId, delayMS
        // 试读出参数
        int64_t delayMS = 0;
        if (int r = dr.Read(clientId, delayMS)) {
            Close(-33, xx::ToString(__LINE__, " SPeer ReceiveCommand Read 'kick' failed. serverId = ", serverId, ", r = ", r).c_str());
            return;
        }
        // 如果没找到 或已断开 则返回，忽略错误
        auto &&cp = TryGetCPeer(clientId);
        if (!cp) {
            // 未找到
            LOG_INFO("SPeer ReceiveCommand 'kick'. TryGetCPeer failed. serverId = ", serverId, ", clientId = ", clientId);
            return;
        }

        // 延迟踢下线
        if (delayMS) {
            LOG_INFO("SPeer ReceiveCommand 'kick' DelayClose. serverId = ", serverId, ", clientId = ", clientId, ", delayMS = ", delayMS);
            // 下发一个 close 指令以便 client 收到后直接主动断开, 响应会比较快速
            cp->SendCommand("close", (uint32_t)0);
            cp->Flush();
            // 会立即从 cps 移除并向白名单 serverIds 对应 peer 广播断开通知. 这之后不再处理收到的消息, 直到超时自杀
            cp->DelayClose((double) delayMS / 1000);
        } else {
            // 立刻踢下线
            cp->Close(-34, xx::ToString(__LINE__, " SPeer ReceiveCommand 'kick' Close. serverId = ", serverId, ", clientId = ", clientId).c_str());
        }
    } else if (cmd == "ping") {                 // ping 的回包. 不关心内容
        // 清除 等回包 状态. 该状态在 pingtimer 中设置，并同时发送 ping 指令
        waitingPingBack = false;
        // 计算延迟, 便于统计
        pingMS = xx::NowSteadyEpochMilliseconds() - lastSendPingMS;
    } else {
        // 收到没有处理函数对应的指令
        Close(-35, xx::ToString(__LINE__, " SPeer ReceiveCommand unhandled cmd. serverId = ", serverId, ", cmd = ", cmd).c_str());
    }
}

CPeer *SPeer::TryGetCPeer(uint32_t const &clientId) {
    auto &&cps = ((Server*)ec)->cps;
    // 根据 client id 找。找不到就返回空指针
    auto &&iter = cps.find(clientId);
    if (iter == cps.end()) return nullptr;
    return &*iter->second;
}
