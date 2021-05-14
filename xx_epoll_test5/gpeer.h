#pragma once

#include "peer.h"

namespace EP = xx::Epoll;
struct VPeer;

// gateway 连进来产生的 peer
struct GPeer : Peer {
    using Peer::Peer;

    uint32_t gatewayId = 0xFFFFFFFFu;
    std::unordered_set<uint32_t> clientIds;

    bool Close(int const& reason, std::string_view const& desc) override;
    void ReceivePackage(uint32_t const& clientId, uint8_t* const& buf, size_t const& len) override;
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;

    void SendOpen(uint32_t const &clientId);
    void SendKick(uint32_t const &clientId, int64_t const& delayMS = 2000);
    void SendClose(uint32_t const &clientId);

    VPeer* GetVPeerByClientId(uint32_t const &clientId) const;
};
