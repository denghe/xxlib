#include "server.h"
#include "speer.h"
#include "vpeer.h"
#include "pkg_db_service.h"

#define S ((Server*)ec)

bool SPeer::Close(int const &reason, std::string_view const &desc) {
    // close fd, ClearCallbacks, DelayUnhold
    if (!this->Peer::Close(reason, desc)) return false;

    if (info) {
        // remove info from mappings
        for (auto const &gi : info->gameInfos) {
            assert(S->gameIdserviceIdMappings.contains(gi.gameId));
            S->gameIdserviceIdMappings.erase(gi.gameId);
        }
        // remove this from sps
        assert(S->sps.contains(info->serviceId));
        S->sps.erase(info->serviceId);

        // rebuild cache
        S->Fill_data_Lobby_Client_GameOpen();
    }

    return true;
}

// package convert helper
#define CASE(T) case xx::TypeId_v<T>: { \
    auto&& o = S->om.As<T>(ob);
#define CASEEND }

void SPeer::ReceivePush(xx::ObjBase_s &&ob) {
    if (info) {
        // registered
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
    else {
        LOG_ERR("unhandled package: ", S->om.ToString(ob));
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
                // check serviceId exists
                if (S->sps.find(o->serviceId) != S->sps.end()) {
                    SendResponse<Generic::Error>(serial, __LINE__, xx::ToString("duplicate serviceId: ", o->serviceId));
                    return;
                }
                // check gameId exists
                for(auto const& gi : o->gameInfos) {
                    if (S->gameIdserviceIdMappings.find(gi.gameId) != S->gameIdserviceIdMappings.end()) {
                        SendResponse<Generic::Error>(serial, __LINE__, xx::ToString("duplicate gameId: ", o->serviceId));
                        return;
                    }
                }

                // combine to mappings
                for(auto const& gi : o->gameInfos) {
                    S->gameIdserviceIdMappings[gi.gameId] = info->serviceId;
                }

                // store peer
                S->sps[o->serviceId] = xx::SharedFromThis(this);

                // store info
                info = std::move(o);

                // regenerate package data
                S->Fill_data_Lobby_Client_GameOpen();

                // response success
                SendResponse<Generic::Success>(serial);
                return;
            CASEEND
            default:
                LOG_ERR("unhandled package: ", S->om.ToString(ob));
                break;
        }
    }
}
