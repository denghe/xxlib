#include "server.h"
#include "dbpeer.h"
#include "pkg_db_service.h"

#define S ((Server*)ec)

bool DBPeer::Close(int const &reason, std::string_view const &desc) {
    if (!this->Peer::Close(reason, desc)) return false;
    S->dbPeer.Reset();
    return true;
}

void DBPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}

void DBPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
