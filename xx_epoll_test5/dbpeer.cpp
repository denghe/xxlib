#include "server.h"
#include "dbpeer.h"
#include "pkg_db.h"

#define S ((Server*)ec)

void DBPeer::ReceivePush(xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}

void DBPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
