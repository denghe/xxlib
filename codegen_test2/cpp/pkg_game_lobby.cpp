#include "pkg_game_lobby.h"
#include "pkg_game_lobby.cpp.inc"
void CodeGen_pkg_game_lobby::Register() {
	::xx::ObjManager::Register<::Lobby_Game::PlayerEnter>();
	::xx::ObjManager::Register<::Lobby_Game::PlayerLeave>();
	::xx::ObjManager::Register<::Game_Lobby::Register>();
	::xx::ObjManager::Register<::Game_Lobby::PlayerLeave>();
}
namespace Lobby_Game{
    void PlayerEnter::WriteTo(xx::Data& d, uint32_t const& gatewayId, uint32_t const& clientId, int32_t const& gameId, ::Generic::PlayerInfo const& playerInfo) {
        d.Write(xx::TypeId_v<PlayerEnter>);
        d.Write(gatewayId);
        d.Write(clientId);
        d.Write(gameId);
        d.Write(playerInfo);
    }
    void PlayerEnter::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gatewayId);
        d.Write(this->clientId);
        d.Write(this->gameId);
        d.Write(this->playerInfo);
    }
    int PlayerEnter::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gatewayId)) return r;
        if (int r = d.Read(this->clientId)) return r;
        if (int r = d.Read(this->gameId)) return r;
        if (int r = d.Read(this->playerInfo)) return r;
        return 0;
    }
    void PlayerEnter::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":210");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerEnter::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gatewayId\":", this->gatewayId);
        om.Append(s, ",\"clientId\":", this->clientId);
        om.Append(s, ",\"gameId\":", this->gameId);
        om.Append(s, ",\"playerInfo\":", this->playerInfo);
#endif
    }
    void PlayerEnter::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Game::PlayerEnter*)tar;
        om.Clone_(this->gatewayId, out->gatewayId);
        om.Clone_(this->clientId, out->clientId);
        om.Clone_(this->gameId, out->gameId);
        om.Clone_(this->playerInfo, out->playerInfo);
    }
    int PlayerEnter::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gatewayId)) return r;
        if (int r = om.RecursiveCheck(this->clientId)) return r;
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        if (int r = om.RecursiveCheck(this->playerInfo)) return r;
        return 0;
    }
    void PlayerEnter::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gatewayId);
        om.RecursiveReset(this->clientId);
        om.RecursiveReset(this->gameId);
        om.RecursiveReset(this->playerInfo);
    }
    void PlayerEnter::SetDefaultValue(::xx::ObjManager& om) {
        this->gatewayId = 0;
        this->clientId = 0;
        this->gameId = 0;
        om.SetDefaultValue(this->playerInfo);
    }
}
namespace Lobby_Game{
    void PlayerLeave::WriteTo(xx::Data& d, int32_t const& accountId, std::string_view const& reason) {
        d.Write(xx::TypeId_v<PlayerLeave>);
        d.Write(accountId);
        d.Write(reason);
    }
    void PlayerLeave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->accountId);
        d.Write(this->reason);
    }
    int PlayerLeave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->accountId)) return r;
        if (int r = d.Read(this->reason)) return r;
        return 0;
    }
    void PlayerLeave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":211");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerLeave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountId\":", this->accountId);
        om.Append(s, ",\"reason\":", this->reason);
#endif
    }
    void PlayerLeave::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Game::PlayerLeave*)tar;
        om.Clone_(this->accountId, out->accountId);
        om.Clone_(this->reason, out->reason);
    }
    int PlayerLeave::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        if (int r = om.RecursiveCheck(this->reason)) return r;
        return 0;
    }
    void PlayerLeave::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountId);
        om.RecursiveReset(this->reason);
    }
    void PlayerLeave::SetDefaultValue(::xx::ObjManager& om) {
        this->accountId = 0;
        om.SetDefaultValue(this->reason);
    }
}
namespace Game_Lobby{
    void Register::WriteTo(xx::Data& d, uint32_t const& serviceId, ::std::vector<::Generic::GameInfo> const& gameInfos) {
        d.Write(xx::TypeId_v<Register>);
        d.Write(serviceId);
        d.Write(gameInfos);
    }
    void Register::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->serviceId);
        d.Write(this->gameInfos);
    }
    int Register::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->serviceId)) return r;
        if (int r = d.Read(this->gameInfos)) return r;
        return 0;
    }
    void Register::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":200");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Register::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"serviceId\":", this->serviceId);
        om.Append(s, ",\"gameInfos\":", this->gameInfos);
#endif
    }
    void Register::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game_Lobby::Register*)tar;
        om.Clone_(this->serviceId, out->serviceId);
        om.Clone_(this->gameInfos, out->gameInfos);
    }
    int Register::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->serviceId)) return r;
        if (int r = om.RecursiveCheck(this->gameInfos)) return r;
        return 0;
    }
    void Register::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->serviceId);
        om.RecursiveReset(this->gameInfos);
    }
    void Register::SetDefaultValue(::xx::ObjManager& om) {
        this->serviceId = 0;
        om.SetDefaultValue(this->gameInfos);
    }
}
namespace Game_Lobby{
    void PlayerLeave::WriteTo(xx::Data& d, int32_t const& accountId, int32_t const& gameId) {
        d.Write(xx::TypeId_v<PlayerLeave>);
        d.Write(accountId);
        d.Write(gameId);
    }
    void PlayerLeave::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->accountId);
        d.Write(this->gameId);
    }
    int PlayerLeave::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->accountId)) return r;
        if (int r = d.Read(this->gameId)) return r;
        return 0;
    }
    void PlayerLeave::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":201");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerLeave::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountId\":", this->accountId);
        om.Append(s, ",\"gameId\":", this->gameId);
#endif
    }
    void PlayerLeave::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Game_Lobby::PlayerLeave*)tar;
        om.Clone_(this->accountId, out->accountId);
        om.Clone_(this->gameId, out->gameId);
    }
    int PlayerLeave::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        return 0;
    }
    void PlayerLeave::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountId);
        om.RecursiveReset(this->gameId);
    }
    void PlayerLeave::SetDefaultValue(::xx::ObjManager& om) {
        this->accountId = 0;
        this->gameId = 0;
    }
}
