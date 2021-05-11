#include "pkg_lobby.h"
#include "pkg_lobby.cpp.inc"
void CodeGen_pkg_lobby::Register() {
	::xx::ObjManager::Register<::Lobby_Client::AuthResult>();
	::xx::ObjManager::Register<::Client_Lobby::Auth>();
}
namespace Lobby_Client{
    void AuthResult::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->accountId);
    }
    int AuthResult::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->accountId)) return r;
        return 0;
    }
    void AuthResult::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":11");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void AuthResult::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountId\":", this->accountId);
#endif
    }
    void AuthResult::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::AuthResult*)tar;
        om.Clone_(this->accountId, out->accountId);
    }
    int AuthResult::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        return 0;
    }
    void AuthResult::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountId);
    }
    void AuthResult::SetDefaultValue(::xx::ObjManager& om) {
        this->accountId = 0;
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
