#include "ldialer.h"
#include "lpeer.h"
#include "server.h"
#include "xx_logger.h"
#include "pkg_game_lobby.h"

#define S ((Server*)ec)

void LDialer::Connect(xx::Shared<LPeer> const &peer) {
    // 没连上
    if (!peer) {
        LOG_INFO("failed");
        return;
    }

    LOG_INFO("success");

    // 加持
    peer->Hold();

    // 将 peer 放入容器
    ((Server *)ec)->lobbyPeer = peer;

    // fill game info
    {
        auto &&o = S->FromCache<Game_Lobby::Register>();
        o->serviceId = 1;
//        {
//            auto &&gi = o->gameInfos.emplace_back();
//            gi.gameId = 1;
//            gi.info = "game1";
//        }
//        {
//            auto &&gi = o->gameInfos.emplace_back();
//            gi.gameId = 2;
//            gi.info = "game2";
//        }
    }

    // send register ?
    //peer->SendRequest
}
