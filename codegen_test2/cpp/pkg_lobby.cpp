#include "pkg_lobby.h"
#include "pkg_lobby.cpp.inc"
void CodeGen_pkg_lobby::Register() {
	::xx::ObjManager::Register<::Lobby_Client::Auth::Online>();
	::xx::ObjManager::Register<::Lobby_Client::Auth::Error>();
	::xx::ObjManager::Register<::Lobby_Client::Auth::Restore>();
	::xx::ObjManager::Register<::Client_Lobby::Auth>();
}
namespace Lobby_Client::Auth{
    void Online::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->accountId);
    }
    int Online::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->accountId)) return r;
        return 0;
    }
    void Online::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":12");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Online::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountId\":", this->accountId);
#endif
    }
    void Online::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Lobby_Client::Auth::Online*)tar;
        om.Clone_(this->accountId, out->accountId);
    }
    int Online::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountId)) return r;
        return 0;
    }
    void Online::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountId);
    }
    void Online::SetDefaultValue(::xx::ObjManager& om) {
        this->accountId = 0;
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
    void Restore::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        this->BaseType::Write(om, d);
        om.Write(d, this->serviceId);
    }
    int Restore::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = this->BaseType::Read(om, d)) return r;
        if (int r = om.Read(d, this->serviceId)) return r;
        return 0;
    }
    void Restore::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":13");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Restore::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        this->BaseType::AppendCore(om, s);
        om.Append(s, ",\"serviceId\":", this->serviceId);
#endif
    }
    void Restore::Clone(::xx::ObjManager& om, void* const &tar) const {
        this->BaseType::Clone(om, tar);
        auto out = (::Lobby_Client::Auth::Restore*)tar;
        om.Clone_(this->serviceId, out->serviceId);
    }
    int Restore::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = this->BaseType::RecursiveCheck(om)) return r;
        if (int r = om.RecursiveCheck(this->serviceId)) return r;
        return 0;
    }
    void Restore::RecursiveReset(::xx::ObjManager& om) {
        this->BaseType::RecursiveReset(om);
        om.RecursiveReset(this->serviceId);
    }
    void Restore::SetDefaultValue(::xx::ObjManager& om) {
        this->BaseType::SetDefaultValue(om);
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
