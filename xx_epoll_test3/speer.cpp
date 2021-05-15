#include "server.h"
#include "speer.h"
#include "pkg_db_service.h"

#define S ((Server*)ec)

bool SPeer::Close(int const &reason, std::string_view const &desc) {
    if (!this->Peer::Close(reason, desc)) return false;
    //S->dbPeer.Reset();
    // todo: remove from mapping game
    return true;
}

void SPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}

void SPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
