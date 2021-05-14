#pragma once

#include "xx_epoll.h"
namespace EP = xx::Epoll;

// gateway 连进来产生的 peer
struct GPeer : EP::TcpPeer {
    using EP::TcpPeer::TcpPeer;

    uint32_t gatewayId = 0xFFFFFFFFu;
    std::unordered_set<uint32_t> clientIds;

    bool Close(int const& reason, std::string_view const& desc) override;
    void Receive() override;

    void ReceivePackage(uint32_t const& clientId, uint8_t* const& buf, size_t const& len);
    void ReceiveCommand(uint8_t* const& buf, size_t const& len);

    // 构造内部指令包. LEN + ADDR + cmd string + args...
    template<typename...Args>
    void SendTo(uint32_t const& addr, Args const &... cmdAndArgs) {
        xx::Data d;
        d.Reserve(32);
        auto bak = d.WriteJump(sizeof(uint32_t));
        d.WriteFixed(addr);
        d.Write(cmdAndArgs...);
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        this->Send(std::move(d));
    }

    void SendOpen(uint32_t const &clientId);
    void SendKick(uint32_t const &clientId, int64_t const& delayMS = 2000);
    void SendClose(uint32_t const &clientId);

    VPeer* GetVPeerByClientId(uint32_t const &clientId) const;
};
