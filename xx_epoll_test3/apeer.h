#pragma once
#include "peer.h"
namespace EP = xx::Epoll;

// anonymous peer. gpeer / speer / ... switcher
struct APeer : Peer {
    using Peer::Peer;
    bool Close(int const& reason, std::string_view const& desc) override;
    void ReceivePackage(uint32_t const& addr, uint8_t* const& buf, size_t const& len) override;
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;
};
