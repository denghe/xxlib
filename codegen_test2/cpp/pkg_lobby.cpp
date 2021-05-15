#include "pkg_lobby.h"
#include "pkg_lobby.cpp.inc"
void CodeGen_pkg_lobby::Register() {
	::xx::ObjManager::Register<::Lobby_Client::Auth::Error>();
	::xx::ObjManager::Register<::Lobby_Client::Auth::Success>();
	::xx::ObjManager::Register<::Client_Lobby::Auth>();
}
namespace xx {
	void ObjFuncs<::Lobby_Client::Game, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::Lobby_Client::Game const& in) {
        om.Write(d, in.gameId);
    }
	void ObjFuncs<::Lobby_Client::Game, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::Lobby_Client::Game const& in) {
        om.Write<false>(d, in.gameId);
    }
	int ObjFuncs<::Lobby_Client::Game, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::Lobby_Client::Game& out) {
        if (int r = om.Read(d, out.gameId)) return r;
        return 0;
    }
	void ObjFuncs<::Lobby_Client::Game, void>::Append(ObjManager &om, std::string& s, ::Lobby_Client::Game const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::Lobby_Client::Game, void>::AppendCore(ObjManager &om, std::string& s, ::Lobby_Client::Game const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"gameId\":", in.gameId); 
#endif
    }
    void ObjFuncs<::Lobby_Client::Game>::Clone(::xx::ObjManager& om, ::Lobby_Client::Game const& in, ::Lobby_Client::Game &out) {
        om.Clone_(in.gameId, out.gameId);
    }
    int ObjFuncs<::Lobby_Client::Game>::RecursiveCheck(::xx::ObjManager& om, ::Lobby_Client::Game const& in) {
        if (int r = om.RecursiveCheck(in.gameId)) return r;
        return 0;
    }
    void ObjFuncs<::Lobby_Client::Game>::RecursiveReset(::xx::ObjManager& om, ::Lobby_Client::Game& in) {
        om.RecursiveReset(in.gameId);
    }
    void ObjFuncs<::Lobby_Client::Game>::SetDefaultValue(::xx::ObjManager& om, ::Lobby_Client::Game& in) {
        in.gameId = 0;
    }
}
namespace Lobby_Client::Auth{
    void Error::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
    }
    int Error::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        return 0;
    }
    void Error::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":11");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Error::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
#endif
    }
    void Error::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
    }
    int Error::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        return 0;
    }
    void Error::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
    }
    void Error::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
    }
}
namespace Lobby_Client::Auth{
    void Success::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->accountId);
        om.Write(d, this->nickname);
        om.Write(d, this->coin);
        om.Write(d, this->games);
        om.Write(d, this->gameId);
        om.Write(d, this->serviceId);
    }
    int Success::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->accountId)) return r;
        if (int r = om.Read(d, this->nickname)) return r;
        if (int r = om.Read(d, this->coin)) return r;
        if (int r = om.Read(d, this->games)) return r;
        if (int r = om.Read(d, this->gameId)) return r;
        if (int r = om.Read(d, this->serviceId)) return r;
        return 0;
    }
    void Success::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":12");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Success::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountId\":", this->accountId);
        om.Append(s, ",\"nickname\":", this->nickname);
        om.Append(s, ",\"coin\":", this->coin);
        om.Append(s, ",\"games\":", this->games);
        om.Append(s, ",\"gameId\":", this->gameId);
        om.Append(s, ",\"serviceId\":", this->serviceId);
#endif
    }
    void Success::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::Auth::Success*)tar;
        om.Clone_(this->accountId, out->accountId);
        om.Clone_(this->nickname, out->nickname);
        om.Clone_(this->coin, out->coin);
        om.Clone_(this->games, out->games);
        om.Clone_(this->gameId, out->gameId);
        om.Clone_(this->serviceId, out->serviceId);
    }
    int Success::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        if (int r = om.RecursiveCheck(this->nickname)) return r;
        if (int r = om.RecursiveCheck(this->coin)) return r;
        if (int r = om.RecursiveCheck(this->games)) return r;
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        if (int r = om.RecursiveCheck(this->serviceId)) return r;
        return 0;
    }
    void Success::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountId);
        om.RecursiveReset(this->nickname);
        om.RecursiveReset(this->coin);
        om.RecursiveReset(this->games);
        om.RecursiveReset(this->gameId);
        om.RecursiveReset(this->serviceId);
    }
    void Success::SetDefaultValue(::xx::ObjManager& om) {
        this->accountId = 0;
        om.SetDefaultValue(this->nickname);
        this->coin = 0;
        om.SetDefaultValue(this->games);
        this->gameId = 0;
        this->serviceId = 0;
    }
}
namespace Client_Lobby{
    void Auth::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->username);
        om.Write(d, this->password);
    }
    int Auth::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->username)) return r;
        if (int r = om.Read(d, this->password)) return r;
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
