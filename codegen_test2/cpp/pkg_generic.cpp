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
        om.Write(d, this->errNumber);
        om.Write(d, this->errMessage);
    }
    int Error::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
        if (int r = om.Read(d, this->errNumber)) return r;
        if (int r = om.Read(d, this->errMessage)) return r;
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
        om.Append(s, ",\"errNumber\":", this->errNumber);
        om.Append(s, ",\"errMessage\":", this->errMessage);
#endif
    }
    void Error::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (::Generic::Error*)tar;
        om.Clone_(this->errNumber, out->errNumber);
        om.Clone_(this->errMessage, out->errMessage);
    }
    int Error::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->errNumber)) return r;
        if (int r = om.RecursiveCheck(this->errMessage)) return r;
        return 0;
    }
    void Error::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->errNumber);
        om.RecursiveReset(this->errMessage);
    }
    void Error::SetDefaultValue(::xx::ObjManager& om) {
        this->errNumber = 0;
        om.SetDefaultValue(this->errMessage);
    }
}
