#include "server.h"
#include "speer.h"
#include "vpeer.h"
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
            auto idx = S->vps.Find<1>(o->accountId);
            if (idx != -1) {
                auto& vp = S->vps.ValueAt(idx);
                assert(vp && vp->info.accountId == o->accountId);
                if (vp->serviceId == info->serviceId && vp->gameId == o->gameId) {
                    vp->serviceId = -1;
                    vp->gameId = -1;
                }
            }
            return;
        CASEEND
        default:
            LOG_ERR("unhandled package: ", S->om.ToString(ob));
            break;
    }
}

void SPeer::ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) {
    if (info) {
        // registered
        LOG_ERR("unhandled package: ", S->om.ToString(ob));
    }
    else {
        switch(ob.typeId()) {
            CASE(Game_Lobby::Register)
                // check exists
                if (S->sps.find(o->serviceId) != S->sps.end()) {
                    auto&& m = InstanceOf<Generic::Error>();
                    m->errorCode = __LINE__;
                    m->errorMessage = xx::ToString("duplicate serviceId: ", o->serviceId);
                    SendResponse(serial, m);
                    return;
                }
                // check gameId has duplicates
                for(auto const& gi : o->gameInfos) {
                    if (S->gameIdserviceIdMappings.find(gi.gameId) != S->gameIdserviceIdMappings.end()) {
                        auto&& m = InstanceOf<Generic::Error>();
                        m->errorCode = __LINE__;
                        m->errorMessage = xx::ToString("duplicate gameId: ", gi.gameId);
                        SendResponse(serial, m);
                        return;
                    }
                }
                // put this to container
                S->sps[o->serviceId] = xx::SharedFromThis(this);
                // todo: sync gameIdserviceIdMappings && cache_gameOpen

                // store info
                info = std::move(o);
                return;
            CASEEND
            default:
                LOG_ERR("unhandled package: ", S->om.ToString(ob));
                break;
        }
    }
}
