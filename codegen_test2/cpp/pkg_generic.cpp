#include "pkg_generic.h"
#include "pkg_generic.cpp.inc"
void CodeGen_pkg_generic::Register() {
	::xx::ObjManager::Register<::Generic::Success>();
	::xx::ObjManager::Register<::Generic::Error>();
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
