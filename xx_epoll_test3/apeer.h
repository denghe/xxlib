#pragma once
#include "peer.h"
namespace EP = xx::Epoll;

// anonymous peer. gpeer / speer / ... switcher
struct APeer : Peer {
    using Peer::Peer;
    bool Close(int const& reason, char const* const& desc) override;
    void ReceivePackage(uint8_t* const& buf, size_t const& len) override;
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;
};
