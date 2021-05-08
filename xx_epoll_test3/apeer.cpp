#include "apeer.h"
#include "server.h"
#include "xx_logger.h"
#include "config.h"
#include "gpeer.h"
// todo: speer, gpeer

bool APeer::Close(int const &reason, std::string_view const &desc) {
    if (!this->Peer::Close(reason, desc)) return false;
    LOG_INFO("reason = ", reason, ", desc = ", desc);
    DelayUnhold();
    return true;
}

void APeer::ReceivePackage(uint32_t const &addr, uint8_t *const &buf, size_t const &len) {
    // should not receive package
    Close(__LINE__, xx::ToString("APeer ReceivePackage addr = ", addr, " buf = ", xx::Span(buf, len)));
}

void APeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    auto &s = *(Server *) ec;
    xx::Data_r dr(buf, len);
    std::string_view cmd;
    if (int r = dr.Read(cmd)) {
        Close(__LINE__, xx::ToString("APeer ReceiveCommand if (int r = dr.Read(cmd)), r = ", r));
        return;
    }
    if (cmd == "gatewayId") {
        uint32_t gatewayId = 0;
        if (int r = dr.Read(gatewayId)) {
            Close(__LINE__, xx::ToString("APeer ReceiveCommand if (int r = dr.Read(gatewayId)), r = ", r));
            return;
        }
        auto iter = s.gps.find(gatewayId);
        if (iter == s.gps.end()) {
            // apeer -> gpeer. known issue: left recv data will be lost. maybe need some shake message
            auto gp = xx::Make<GPeer>(ec);
            this->SwapFD(gp);
            gp->Hold();
            this->DelayUnhold();

            gp->SetTimeoutSeconds(config.peerTimeoutSeconds);
            gp->gatewayId = gatewayId;
            s.gps[gatewayId] = gp;
            LOG_INFO("cmd gatewayId = ", gatewayId, " cmd = gatewayId, success");
        } else {
            Close(__LINE__, xx::ToString("APeer ReceiveCommand cmd gatewayId = ", gatewayId, " already exists"));
        }
        return;
    } else if (cmd == "serverId") {
        uint32_t serverId = 0;
        if (int r = dr.Read(serverId)) {
            Close(__LINE__, xx::ToString("APeer ReceiveCommand cmd serverId if (int r = dr.Read(serverId)), r = ", r));
            return;
        }
        switch (serverId) {
            case 1: // game 1
            {
                LOG_INFO("serverId = ", serverId, " cmd = register");
                // todo: create game peer, swap fd, put peer to server.sps[serverId]
                return;
            }
                // ....
            default:;
        }
        Close(__LINE__, xx::ToString("APeer ReceiveCommand unhandled cmd serverId = ", serverId));
        return;
    }
    Close(__LINE__, xx::ToString("APeer ReceiveCommand unhandled cmd = ", cmd));
}
