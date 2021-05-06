#pragma once

#include "peer.h"

namespace EP = xx::Epoll;

// gateway 连进来产生的 peer
struct GPeer : Peer {
    using Peer::Peer;

    uint32_t gatewayId = 0xFFFFFFFFu;
    std::unordered_set<uint32_t> clientIds;

    bool Close(int const& reason, char const* const& desc) override;
    void ReceivePackage(uint8_t* const& buf, size_t const& len) override;
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;

    void SendOpen(uint32_t const &clientId);
    void SendKick(uint32_t const &clientId);
    void SendClose(uint32_t const &clientId);
};
