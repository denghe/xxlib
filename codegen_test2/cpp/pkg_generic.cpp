#include "pkg_generic.h"
#include "pkg_generic.cpp.inc"
void CodeGen_pkg_generic::Register() {
	::xx::ObjManager::Register<::Generic::Success>();
	::xx::ObjManager::Register<::Generic::Error>();
	::xx::ObjManager::Register<::Generic::GameInfo>();
}
namespace xx {
	void ObjFuncs<::Generic::PlayerInfo, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::Generic::PlayerInfo const& in) {
        om.Write(d, in.accountId);
        om.Write(d, in.nickname);
        om.Write(d, in.coin);
    }
	void ObjFuncs<::Generic::PlayerInfo, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::Generic::PlayerInfo const& in) {
        om.Write<false>(d, in.accountId);
        om.Write<false>(d, in.nickname);
        om.Write<false>(d, in.coin);
    }
	int ObjFuncs<::Generic::PlayerInfo, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::Generic::PlayerInfo& out) {
        if (int r = om.Read(d, out.accountId)) return r;
        if (int r = om.Read(d, out.nickname)) return r;
        if (int r = om.Read(d, out.coin)) return r;
        return 0;
    }
	void ObjFuncs<::Generic::PlayerInfo, void>::Append(ObjManager &om, std::string& s, ::Generic::PlayerInfo const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::Generic::PlayerInfo, void>::AppendCore(ObjManager &om, std::string& s, ::Generic::PlayerInfo const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"accountId\":", in.accountId); 
        om.Append(s, ",\"nickname\":", in.nickname);
        om.Append(s, ",\"coin\":", in.coin);
#endif
    }
    void ObjFuncs<::Generic::PlayerInfo>::Clone(::xx::ObjManager& om, ::Generic::PlayerInfo const& in, ::Generic::PlayerInfo &out) {
        om.Clone_(in.accountId, out.accountId);
        om.Clone_(in.nickname, out.nickname);
        om.Clone_(in.coin, out.coin);
    }
    int ObjFuncs<::Generic::PlayerInfo>::RecursiveCheck(::xx::ObjManager& om, ::Generic::PlayerInfo const& in) {
        if (int r = om.RecursiveCheck(in.accountId)) return r;
        if (int r = om.RecursiveCheck(in.nickname)) return r;
        if (int r = om.RecursiveCheck(in.coin)) return r;
        return 0;
    }
    void ObjFuncs<::Generic::PlayerInfo>::RecursiveReset(::xx::ObjManager& om, ::Generic::PlayerInfo& in) {
        om.RecursiveReset(in.accountId);
        om.RecursiveReset(in.nickname);
        om.RecursiveReset(in.coin);
    }
    void ObjFuncs<::Generic::PlayerInfo>::SetDefaultValue(::xx::ObjManager& om, ::Generic::PlayerInfo& in) {
        in.accountId = 0;
        om.SetDefaultValue(in.nickname);
        in.coin = 0;
    }
}
namespace Generic{
    void Success::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    }
    int Success::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        return 0;
    }
    void Success::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":1");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Success::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
#endif
    }
    void Success::Clone(::xx::ObjManager& om, void* const &tar) const {
    }
    int Success::RecursiveCheck(::xx::ObjManager& om) const {
        return 0;
    }
    void Success::RecursiveReset(::xx::ObjManager& om) {
    }
    void Success::SetDefaultValue(::xx::ObjManager& om) {
    }
}
namespace Generic{
    void Error::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->errorCode);
        om.Write(d, this->errorMessage);
    }
    int Error::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->errorCode)) return r;
        if (int r = om.Read(d, this->errorMessage)) return r;
        return 0;
    }
    void Error::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":2");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void Error::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"errorCode\":", this->errorCode);
        om.Append(s, ",\"errorMessage\":", this->errorMessage);
#endif
    }
    void Error::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::Error*)tar;
        om.Clone_(this->errorCode, out->errorCode);
        om.Clone_(this->errorMessage, out->errorMessage);
    }
    int Error::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->errorCode)) return r;
        if (int r = om.RecursiveCheck(this->errorMessage)) return r;
        return 0;
    }
    void Error::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->errorCode);
        om.RecursiveReset(this->errorMessage);
    }
    void Error::SetDefaultValue(::xx::ObjManager& om) {
        this->errorCode = 0;
        om.SetDefaultValue(this->errorMessage);
    }
}
namespace Generic{
    void GameInfo::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->gameId);
        om.Write(d, this->info);
    }
    int GameInfo::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->gameId)) return r;
        if (int r = om.Read(d, this->info)) return r;
        return 0;
    }
    void GameInfo::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":3");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GameInfo::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"gameId\":", this->gameId);
        om.Append(s, ",\"info\":", this->info);
#endif
    }
    void GameInfo::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::GameInfo*)tar;
        om.Clone_(this->gameId, out->gameId);
        om.Clone_(this->info, out->info);
    }
    int GameInfo::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->gameId)) return r;
        if (int r = om.RecursiveCheck(this->info)) return r;
        return 0;
    }
    void GameInfo::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->gameId);
        om.RecursiveReset(this->info);
    }
    void GameInfo::SetDefaultValue(::xx::ObjManager& om) {
        this->gameId = 0;
        om.SetDefaultValue(this->info);
    }
}
