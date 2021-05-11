#include "server.h"
#include "gpeer.h"
#include "vpeer.h"
#include "config.h"
#include "xx_logger.h"

#define S ((Server*)ec)

void GPeer::SendOpen(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "open", clientId);
}

void GPeer::SendKick(uint32_t const &clientId, int64_t const &delayMS) {
    LOG_INFO("clientId = ", clientId, " delayMS = ", delayMS);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "kick", clientId, delayMS);
}

void GPeer::SendClose(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "close", clientId);
}

bool GPeer::Close(int const &reason, std::string_view const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;
    assert(gatewayId != 0xFFFFFFFFu);
    LOG_INFO("gatewayId = ", gatewayId, ", reason = ", reason, ", desc = ", desc);

    // close all child vpeer
    for (auto &&clientId: clientIds) {
        auto p = GetVPeerByClientId(clientId);
        assert(p);
        p->Kick(__LINE__, "GPeer Close", true);
    }

    // 从容器移除 this
    assert(S->gps.find(gatewayId) != S->gps.end());
    S->gps.erase(gatewayId);

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
        LOG_ERR("gatewayId = ", gatewayId, " dr.Read(cmd) r = ", r);
        return;
    }
    if (cmd == "accept") {
        uint32_t clientId;
        if (int r = dr.Read(clientId)) {
            LOG_ERR("gatewayId = ", gatewayId, " accept dr.Read(clientId) r = ", r);
            return;
        }
        std::string ip;
        if (int r = dr.Read(ip)) {
            LOG_ERR("gatewayId = ", gatewayId, " accept dr.Read(ip) r = ", r);
            return;
        }
        // create peer, fill props, hold & store to server.vps, send open
        LOG_INFO("gatewayId = ", gatewayId, " accept clientId = ", clientId, " ip = ", ip);
        // todo: check ip is valid?
        (void)xx::Make<VPeer>(S, this, clientId, std::move(ip));
    } else if (cmd == "close") {
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

VPeer *GPeer::GetVPeerByClientId(uint32_t const &clientId) const {
    assert(gatewayId != 0xFFFFFFFFu);
    assert(clientIds.contains(clientId));
    int idx = S->vps.Find<0>(((uint64_t) gatewayId << 32) | clientId);
    if (idx >= 0) return S->vps.ValueAt(idx).pointer;
    return nullptr;
}
