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

#define CASE(T) case xx::TypeId_v<T>: { auto&& o = S->om.As<T>(ob);
#define CASEEND }

void SPeer::ReceivePush(xx::ObjBase_s &&ob) {
    switch(ob.typeId()) {
        CASE(Game_Lobby::PlayerLeave)
            // todo: find player & remove from game
            //o->accountId
            return;
        CASEEND
        defaut:
            LOG_ERR("unhandled package: ", S->om.ToString(ob));
            break;
    }
}

void SPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    if (info) {
        // registered
//        switch(ob.typeId()) {
//            CASE(Game_Lobby::PlayerLeave)
//
//                return;
//            CASEEND
//            defaut:
//                LOG_ERR("unhandled package: ", S->om.ToString(ob));
//                break;
//        }
    }
    else {
//        switch(ob.typeId()) {
//            CASE(Game_Lobby::PlayerLeave)
//
//                return;
//            CASEEND
//            defaut:
//                LOG_ERR("unhandled package: ", S->om.ToString(ob));
//                break;
//        }
    }
    //LOG_ERR("unhandled package: ", S->om.ToString(ob));
}
