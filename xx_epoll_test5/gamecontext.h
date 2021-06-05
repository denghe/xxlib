#pragma once
#include "server.h"

struct GameContext {
    Server* server;
    explicit GameContext(Server* const& server) : server(server) {

    }

    void Update(double const &dt) {

    }

    // todo: bind vps & player ?
};
