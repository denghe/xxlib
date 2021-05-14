#pragma once
#include "server.h"

struct VPeer;
struct SPeer;

struct Game {
    int gameId = -1;

    xx::Shared<SPeer> peer;

    std::unordered_set<uint32_t> accountIds;
};
