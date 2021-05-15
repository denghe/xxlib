#include "pkg_db_service.h"
#include "pkg_db_service.cpp.inc"
void CodeGen_pkg_db_service::Register() {
	::xx::ObjManager::Register<::Database_Service::GetAccountInfoByUsernamePasswordResult>();
	::xx::ObjManager::Register<::Service_Database::GetAccountInfoByUsernamePassword>();
}
namespace xx {
	void ObjFuncs<::Database::AccountInfo, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::Database::AccountInfo const& in) {
        om.Write(d, in.accountId);
        om.Write(d, in.nickname);
        om.Write(d, in.coin);
    }
	void ObjFuncs<::Database::AccountInfo, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::Database::AccountInfo const& in) {
        om.Write<false>(d, in.accountId);
        om.Write<false>(d, in.nickname);
        om.Write<false>(d, in.coin);
    }
	int ObjFuncs<::Database::AccountInfo, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::Database::AccountInfo& out) {
        if (int r = om.Read(d, out.accountId)) return r;
        if (int r = om.Read(d, out.nickname)) return r;
        if (int r = om.Read(d, out.coin)) return r;
        return 0;
    }
	void ObjFuncs<::Database::AccountInfo, void>::Append(ObjManager &om, std::string& s, ::Database::AccountInfo const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::Database::AccountInfo, void>::AppendCore(ObjManager &om, std::string& s, ::Database::AccountInfo const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"accountId\":", in.accountId); 
        om.Append(s, ",\"nickname\":", in.nickname);
        om.Append(s, ",\"coin\":", in.coin);
#endif
    }
    void ObjFuncs<::Database::AccountInfo>::Clone(::xx::ObjManager& om, ::Database::AccountInfo const& in, ::Database::AccountInfo &out) {
        om.Clone_(in.accountId, out.accountId);
        om.Clone_(in.nickname, out.nickname);
        om.Clone_(in.coin, out.coin);
    }
    int ObjFuncs<::Database::AccountInfo>::RecursiveCheck(::xx::ObjManager& om, ::Database::AccountInfo const& in) {
        if (int r = om.RecursiveCheck(in.accountId)) return r;
        if (int r = om.RecursiveCheck(in.nickname)) return r;
        if (int r = om.RecursiveCheck(in.coin)) return r;
        return 0;
    }
    void ObjFuncs<::Database::AccountInfo>::RecursiveReset(::xx::ObjManager& om, ::Database::AccountInfo& in) {
        om.RecursiveReset(in.accountId);
        om.RecursiveReset(in.nickname);
        om.RecursiveReset(in.coin);
    }
    void ObjFuncs<::Database::AccountInfo>::SetDefaultValue(::xx::ObjManager& om, ::Database::AccountInfo& in) {
        in.accountId = -1;
        om.SetDefaultValue(in.nickname);
        in.coin = 0;
    }
}
namespace Database_Service{
    void GetAccountInfoByUsernamePasswordResult::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->accountInfo);
    }
    int GetAccountInfoByUsernamePasswordResult::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->accountInfo)) return r;
        return 0;
    }
    void GetAccountInfoByUsernamePasswordResult::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":110");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GetAccountInfoByUsernamePasswordResult::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"accountInfo\":", this->accountInfo);
#endif
    }
    void GetAccountInfoByUsernamePasswordResult::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Database_Service::GetAccountInfoByUsernamePasswordResult*)tar;
        om.Clone_(this->accountInfo, out->accountInfo);
    }
    int GetAccountInfoByUsernamePasswordResult::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->accountInfo)) return r;
        return 0;
    }
    void GetAccountInfoByUsernamePasswordResult::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->accountInfo);
    }
    void GetAccountInfoByUsernamePasswordResult::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->accountInfo);
    }
}
namespace Service_Database{
    void GetAccountInfoByUsernamePassword::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->username);
        om.Write(d, this->password);
    }
    int GetAccountInfoByUsernamePassword::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->username)) return r;
        if (int r = om.Read(d, this->password)) return r;
        return 0;
    }
    void GetAccountInfoByUsernamePassword::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":100");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void GetAccountInfoByUsernamePassword::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        om.Append(s, ",\"username\":", this->username);
        om.Append(s, ",\"password\":", this->password);
#endif
    }
    void GetAccountInfoByUsernamePassword::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Service_Database::GetAccountInfoByUsernamePassword*)tar;
        om.Clone_(this->username, out->username);
        om.Clone_(this->password, out->password);
    }
    int GetAccountInfoByUsernamePassword::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->username)) return r;
        if (int r = om.RecursiveCheck(this->password)) return r;
        return 0;
    }
    void GetAccountInfoByUsernamePassword::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->username);
        om.RecursiveReset(this->password);
    }
    void GetAccountInfoByUsernamePassword::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->username);
        om.SetDefaultValue(this->password);
    }
}
