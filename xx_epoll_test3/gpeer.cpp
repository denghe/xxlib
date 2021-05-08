#include "server.h"
#include "gpeer.h"
#include "vpeer.h"
#include "config.h"
#include "xx_logger.h"

void GPeer::SendOpen(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    SendTo(0xFFFFFFFFu, "open", clientId);
}

void GPeer::SendKick(uint32_t const &clientId, int64_t const &delayMS) {
    LOG_INFO("clientId = ", clientId, " delayMS = ", delayMS);
    SendTo(0xFFFFFFFFu, "kick", clientId, delayMS);
}

void GPeer::SendClose(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    SendTo(0xFFFFFFFFu, "close", clientId);
}

bool GPeer::Close(int const &reason, char const *const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;
    assert(gatewayId != 0xFFFFFFFFu);
    LOG_INFO("gatewayId = ", gatewayId, ", reason = ", reason, ", desc = ", desc);

    auto &s = *(Server *) ec;

    // close all child vpeer
    for (auto &&vp: vps) {
        vp->Kick(__LINE__, "GPeer Close");
    }

    // 从容器移除 this
    assert(s.gps.find(gatewayId) != s.gps.end());
    s.gps.erase(gatewayId);

    // set flag
    gatewayId = 0xFFFFFFFFu;

    // 延迟减持
    DelayUnhold();
    return true;
}

void GPeer::ReceivePackage(uint32_t const &clientId, uint8_t *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    LOG_INFO("gatewayId:", gatewayId, ", clientId = , ", clientId, ", buf = ", dr);
    if (auto p = GetVPeerByClientId(clientId)) {
        p->Receive(dr.buf + dr.offset, dr.len - dr.offset);
    }
    // if not find, ignore
}

void GPeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    std::string cmd;
    if (int r = dr.Read(cmd)) {
        LOG_ERR("gatewayId:", gatewayId, " dr.Read(cmd) r = ", r);
        return;
    }
    if (cmd == "close") {
        uint32_t clientId;
        if (int r = dr.Read(clientId)) {
            LOG_ERR("gatewayId = ", gatewayId, " close dr.Read(clientId) r = ", r);
            return;
        }
        if (auto p = GetVPeerByClientId(clientId)) {
            p->Kick(__LINE__, xx::ToString("GPeer ReceiveCommand gatewayId = ", gatewayId, " close clientId = ", clientId));
        }
    } else if (cmd == "ping") {
        // keep alive
        SetTimeoutSeconds(config.peerTimeoutSeconds);
        // echo back
        SendTo(0xFFFFFFFFu, "ping", dr.LeftSpan());
    } else {
        LOG_ERR("gatewayId = ", gatewayId, " unhandled cmd = ", cmd);
    }
}

VPeer* GPeer::GetVPeerByClientId(uint32_t const &clientId) const {
    assert(gatewayId != 0xFFFFFFFFu);
    auto &s = *(Server *) ec;
    int idx = s.vps.Find<0>( ((uint64_t)gatewayId << 32) | clientId );
    if (idx >= 0) {
        auto p = s.vps.ValueAt(idx).pointer;
        assert(vps.contains(p));
        return p;
    }
    return nullptr;
}
