#include "p1.h"
#include "p1.cpp.inc"
void CodeGen_p1::Register() {
	::xx::ObjManager::Register<p1::p1c1>();
}
namespace p1{
    void p1c1::Write(::xx::ObjManager& om, ::xx::Data& d) const {
        om.Write(d, this->a);
    }
    int p1c1::Read(::xx::ObjManager& om, ::xx::Data& d) {
        if (int r = om.Read(d, this->a)) return r;
        return 0;
    }
    void p1c1::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, "{\"__typeId__\":10");
        this->AppendCore(om, s);
        s.push_back('}');
#endif
    }
    void p1c1::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
        ::xx::Append(s, ",\"a\":", this->a);
#endif
    }
    void p1c1::Clone(::xx::ObjManager& om, void* const &tar) const {
        auto out = (p1::p1c1*)tar;
        om.Clone_(this->a, out->a);
    }
    int p1c1::RecursiveCheck(::xx::ObjManager& om) const {
        if (int r = om.RecursiveCheck(this->a)) return r;
        return 0;
    }
    void p1c1::RecursiveReset(::xx::ObjManager& om) {
        om.RecursiveReset(this->a);
    }
    void p1c1::SetDefaultValue(::xx::ObjManager& om) {
        om.SetDefaultValue(this->a);
    }
}
