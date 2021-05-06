#include "server.h"
#include "gpeer.h"
//#include "vpeer.h"
#include "config.h"
#include "xx_logger.h"

void GPeer::SendOpen(uint32_t const &clientId) {
    LOG_INFO("open:", clientId);
    SendTo(0xFFFFFFFFu, "open", clientId);
}

void GPeer::SendKick(uint32_t const &clientId) {
    LOG_INFO("kick:", clientId);
    SendTo(0xFFFFFFFFu, "kick", clientId);
}

void GPeer::SendClose(uint32_t const &clientId) {
    LOG_INFO("close:", clientId);
    SendTo(0xFFFFFFFFu, "close", clientId);
}

bool GPeer::Close(int const &reason, char const *const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;
    assert(gatewayId != 0xFFFFFFFFu);
    LOG_INFO("GPeer Close, gatewayId = ", gatewayId, ", reason = ", reason, ", desc = ", desc);
    // 关闭所有虚拟 peer
    auto server = (Server *) ec;

    for (auto&& clientId: clientIds) {
        //server->DisconnectVPeerByGatewayClientId(gatewayId, clientId);
    }
    clientIds.clear();

    // 从容器移除 this( 如果有放入的话 )
    server->gps.erase(gatewayId);

    // 延迟减持
    DelayUnhold();
    return true;
}

void GPeer::ReceivePackage(uint8_t *const &buf, size_t const &len) {
//        if (auto peer = SP->GetVPeerByGatewayClientId(gatewayId, clientId)) {
//            peer->Receive(dr.buf + dr.offset, dr.len - dr.offset);
//        }
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
            LOG_ERR("gatewayId:", gatewayId, ", cmd: close, dr.Read(clientId) r = ", r);
            return;
        }
        LOG_INFO("gatewayId:", gatewayId, ", cmd: close, client id = ", clientId);
//            if (auto peer = GetVPeerByClientId(clientId)) {
//                peer->Close(__LINE__, xx::ToString("gateway:", gatewayId, " close client id:", clientId).c_str());
//            }
    } else if (cmd == "ping") {
        // keep alive
        SetTimeoutSeconds(config.peerTimeoutSeconds);
        // echo back
        SendTo(0xFFFFFFFFu, "ping", dr.LeftSpan());
    } else {
        LOG_ERR("gatewayId:", gatewayId, " recv unknown cmd: ", cmd);
    }
}
