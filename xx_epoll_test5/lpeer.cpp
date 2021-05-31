#include "server.h"
#include "lpeer.h"
#include "pkg_lobby_client.h"

#define S ((Server*)ec)

bool LPeer::Close(int const &reason, std::string_view const &desc) {
    // close fd, clear callbacks, delay unhold
    if (!this->Peer::Close(reason, desc)) return false;

    // remove from container
    S->lobbyPeer.Reset();

    // todo: maybe need kick all online players ?

    return true;
}

void LPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}

void LPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
