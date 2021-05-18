#include "pkg_lobby_client.h"
#include "pkg_lobby_client.cpp.inc"
void CodeGen_pkg_lobby_client::Register() {
	::xx::ObjManager::Register<::Lobby_Client::PlayerContext>();
	::xx::ObjManager::Register<::Lobby_Client::EnterGameSuccess>();
	::xx::ObjManager::Register<::Lobby_Client::GameOpen>();
	::xx::ObjManager::Register<::Lobby_Client::GameClose>();
	::xx::ObjManager::Register<::Client_Lobby::Auth>();
	::xx::ObjManager::Register<::Client_Lobby::EnterGame>();
}
namespace Lobby_Client{
    void PlayerContext::WriteTo(xx::Data& d, ::Generic::PlayerInfo const& self, int32_t const& gameId, int32_t const& serviceId) {
        d.Write(xx::TypeId_v<PlayerContext>);
        d.Write(self);
        d.Write(gameId);
        d.Write(serviceId);
    }
    void PlayerContext::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->self);
        d.Write(this->gameId);
        d.Write(this->serviceId);
    }
    int PlayerContext::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->self)) return r;
        if (int r = d.Read(this->gameId)) return r;
        if (int r = d.Read(this->serviceId)) return r;
        return 0;
    }
    void PlayerContext::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":20");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void PlayerContext::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"self\":", this->self);
        om.Append(s, ",\"gameId\":", this->gameId);
        om.Append(s, ",\"serviceId\":", this->serviceId);
#endif
    }
    void PlayerContext::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::PlayerContext*)tar;
        om.Clone_(this->self, out->self);
        om.Clone_(this->gameId, out->gameId);
        om.Clone_(this->serviceId, out->serviceId);
    }
    int PlayerContext::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->self)) return r;
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        if (int r = om.RecursiveCheck(this->serviceId)) return r;
        return 0;
    }
    void PlayerContext::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->self);
        om.RecursiveReset(this->gameId);
        om.RecursiveReset(this->serviceId);
    }
    void PlayerContext::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->self);
        this->gameId = 0;
        this->serviceId = 0;
    }
}
namespace Lobby_Client{
    void EnterGameSuccess::WriteTo(xx::Data& d, int32_t const& serviceId) {
        d.Write(xx::TypeId_v<EnterGameSuccess>);
        d.Write(serviceId);
    }
    void EnterGameSuccess::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->serviceId);
    }
    int EnterGameSuccess::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->serviceId)) return r;
        return 0;
    }
    void EnterGameSuccess::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":21");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void EnterGameSuccess::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"serviceId\":", this->serviceId);
#endif
    }
    void EnterGameSuccess::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::EnterGameSuccess*)tar;
        om.Clone_(this->serviceId, out->serviceId);
    }
    int EnterGameSuccess::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->serviceId)) return r;
        return 0;
    }
    void EnterGameSuccess::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->serviceId);
    }
    void EnterGameSuccess::SetDefaultValue(::xx::ObjManager& om) {
        this->serviceId = 0;
    }
}
namespace Lobby_Client{
    void GameOpen::WriteTo(xx::Data& d, ::std::vector<::Generic::GameInfo> const& gameInfos) {
        d.Write(xx::TypeId_v<GameOpen>);
        d.Write(gameInfos);
    }
    void GameOpen::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gameInfos);
    }
    int GameOpen::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gameInfos)) return r;
        return 0;
    }
    void GameOpen::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":30");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GameOpen::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gameInfos\":", this->gameInfos);
#endif
    }
    void GameOpen::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::GameOpen*)tar;
        om.Clone_(this->gameInfos, out->gameInfos);
    }
    int GameOpen::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gameInfos)) return r;
        return 0;
    }
    void GameOpen::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gameInfos);
    }
    void GameOpen::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->gameInfos);
    }
}
namespace Lobby_Client{
    void GameClose::WriteTo(xx::Data& d, ::std::vector<int32_t> const& gameIds) {
        d.Write(xx::TypeId_v<GameClose>);
        d.Write(gameIds);
    }
    void GameClose::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gameIds);
    }
    int GameClose::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gameIds)) return r;
        return 0;
    }
    void GameClose::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":31");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GameClose::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gameIds\":", this->gameIds);
#endif
    }
    void GameClose::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::GameClose*)tar;
        om.Clone_(this->gameIds, out->gameIds);
    }
    int GameClose::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gameIds)) return r;
        return 0;
    }
    void GameClose::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gameIds);
    }
    void GameClose::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->gameIds);
    }
}
namespace Client_Lobby{
    void Auth::WriteTo(xx::Data& d, ::std::string const& username, ::std::string const& password) {
        d.Write(xx::TypeId_v<Auth>);
        d.Write(username);
        d.Write(password);
    }
    void Auth::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->username);
        d.Write(this->password);
    }
    int Auth::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->username)) return r;
        if (int r = d.Read(this->password)) return r;
        return 0;
    }
    void Auth::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":10");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Auth::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"username\":", this->username);
        om.Append(s, ",\"password\":", this->password);
#endif
    }
    void Auth::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Lobby::Auth*)tar;
        om.Clone_(this->username, out->username);
        om.Clone_(this->password, out->password);
    }
    int Auth::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->username)) return r;
        if (int r = om.RecursiveCheck(this->password)) return r;
        return 0;
    }
    void Auth::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->username);
        om.RecursiveReset(this->password);
    }
    void Auth::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->username);
        om.SetDefaultValue(this->password);
    }
}
namespace Client_Lobby{
    void EnterGame::WriteTo(xx::Data& d, int32_t const& gameId) {
        d.Write(xx::TypeId_v<EnterGame>);
        d.Write(gameId);
    }
    void EnterGame::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        d.Write(this->gameId);
    }
    int EnterGame::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = d.Read(this->gameId)) return r;
        return 0;
    }
    void EnterGame::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":11");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void EnterGame::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gameId\":", this->gameId);
#endif
    }
    void EnterGame::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Client_Lobby::EnterGame*)tar;
        om.Clone_(this->gameId, out->gameId);
    }
    int EnterGame::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        return 0;
    }
    void EnterGame::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gameId);
    }
    void EnterGame::SetDefaultValue(::xx::ObjManager& om) {
        this->gameId = 0;
    }
}
