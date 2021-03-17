#include "shared.h"
#include "shared.cpp.inc"
void CodeGen_shared::Register() {
	::xx::ObjManager::Register<::A>();
	::xx::ObjManager::Register<::B>();
}
namespace xx {
	void ObjFuncs<::C, void>::Write(::xx::ObjManager& om, ::C const& in) {
        om.Write(in.x);
        om.Write(in.y);
        om.Write(in.targets);
    }
	int ObjFuncs<::C, void>::Read(::xx::ObjManager& om, ::C& out) {
        if (int r = om.Read(out.x)) return r;
        if (int r = om.Read(out.y)) return r;
        if (int r = om.Read(out.targets)) return r;
        return 0;
    }
	void ObjFuncs<::C, void>::Append(ObjManager &om, ::C const& in) {
#ifndef XX_DISABLE_APPEND
        om.str->push_back('{');
        AppendCore(om, in);
        om.str->push_back('}');
#endif
    }
	void ObjFuncs<::C, void>::AppendCore(ObjManager &om, ::C const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append("\"x\":", in.x); 
        om.Append(",\"y\":", in.y);
        om.Append(",\"targets\":", in.targets);
#endif
    }
    void ObjFuncs<::C>::Clone1(::xx::ObjManager& om, ::C const& in, ::C &out) {
        om.Clone1(in.x, out.x);
        om.Clone1(in.y, out.y);
        om.Clone1(in.targets, out.targets);
    }
    void ObjFuncs<::C>::Clone2(::xx::ObjManager& om, ::C const& in, ::C &out) {
        om.Clone2(in.x, out.x);
        om.Clone2(in.y, out.y);
        om.Clone2(in.targets, out.targets);
    }
    int ObjFuncs<::C>::RecursiveCheck(::xx::ObjManager& om, ::C const& in) {
        if (int r = om.RecursiveCheck(in.x)) return r;
        if (int r = om.RecursiveCheck(in.y)) return r;
        if (int r = om.RecursiveCheck(in.targets)) return r;
        return 0;
    }
    void ObjFuncs<::C>::RecursiveReset(::xx::ObjManager& om, ::C& in) {
        om.RecursiveReset(in.x);
        om.RecursiveReset(in.y);
        om.RecursiveReset(in.targets);
    }
    void ObjFuncs<::C>::SetDefaultValue(::xx::ObjManager& om, ::C& in) {
        in.x = 0.0f;
        in.y = 0.0f;
        om.SetDefaultValue(in.targets);
    }
}
void A::Write(::xx::ObjManager& om) const {
    om.Write(this->id);
    om.Write(this->nick);
    om.Write(this->parent);
    om.Write(this->children);
}
int A::Read(::xx::ObjManager& om) {
    if (int r = om.Read(this->id)) return r;
    if (int r = om.Read(this->nick)) return r;
    if (int r = om.Read(this->parent)) return r;
    if (int r = om.Read(this->children)) return r;
    return 0;
}
void A::Append(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append("{\"__typeId__\":", this->ObjBase::GetTypeId());
    this->AppendCore(om);
    om.str->push_back('}');
#endif
}
void A::AppendCore(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append(",\"id\":", this->id);
    om.Append(",\"nick\":", this->nick);
    om.Append(",\"parent\":", this->parent);
    om.Append(",\"children\":", this->children);
#endif
}
void A::Clone1(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::A*)tar;
    om.Clone1(this->id, out->id);
    om.Clone1(this->nick, out->nick);
    om.Clone1(this->parent, out->parent);
    om.Clone1(this->children, out->children);
}
void A::Clone2(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::A*)tar;
    om.Clone2(this->id, out->id);
    om.Clone2(this->nick, out->nick);
    om.Clone2(this->parent, out->parent);
    om.Clone2(this->children, out->children);
}
int A::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->id)) return r;
    if (int r = om.RecursiveCheck(this->nick)) return r;
    if (int r = om.RecursiveCheck(this->parent)) return r;
    if (int r = om.RecursiveCheck(this->children)) return r;
    return 0;
}
void A::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->id);
    om.RecursiveReset(this->nick);
    om.RecursiveReset(this->parent);
    om.RecursiveReset(this->children);
}
void A::SetDefaultValue(::xx::ObjManager& om) {
    this->id = 0;
    om.SetDefaultValue(this->nick);
    om.SetDefaultValue(this->parent);
    om.SetDefaultValue(this->children);
}
void B::Write(::xx::ObjManager& om) const {
    this->BaseType::Write(om);
    om.Write(this->data);
    om.Write(this->c);
    om.Write(this->c2);
    om.Write(this->c3);
}
int B::Read(::xx::ObjManager& om) {
    if (int r = this->BaseType::Read(om)) return r;
    if (int r = om.Read(this->data)) return r;
    if (int r = om.Read(this->c)) return r;
    if (int r = om.Read(this->c2)) return r;
    if (int r = om.Read(this->c3)) return r;
    return 0;
}
void B::Append(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append("{\"__typeId__\":", this->ObjBase::GetTypeId());
    this->AppendCore(om);
    om.str->push_back('}');
#endif
}
void B::AppendCore(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    this->BaseType::AppendCore(om);
    om.Append(",\"data\":", this->data);
    om.Append(",\"c\":", this->c);
    om.Append(",\"c2\":", this->c2);
    om.Append(",\"c3\":", this->c3);
#endif
}
void B::Clone1(::xx::ObjManager& om, void* const &tar) const {
    this->BaseType::Clone1(om, tar);
    auto out = (::B*)tar;
    om.Clone1(this->data, out->data);
    om.Clone1(this->c, out->c);
    om.Clone1(this->c2, out->c2);
    om.Clone1(this->c3, out->c3);
}
void B::Clone2(::xx::ObjManager& om, void* const &tar) const {
    this->BaseType::Clone2(om, tar);
    auto out = (::B*)tar;
    om.Clone2(this->data, out->data);
    om.Clone2(this->c, out->c);
    om.Clone2(this->c2, out->c2);
    om.Clone2(this->c3, out->c3);
}
int B::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = this->BaseType::RecursiveCheck(om)) return r;
    if (int r = om.RecursiveCheck(this->data)) return r;
    if (int r = om.RecursiveCheck(this->c)) return r;
    if (int r = om.RecursiveCheck(this->c2)) return r;
    if (int r = om.RecursiveCheck(this->c3)) return r;
    return 0;
}
void B::RecursiveReset(::xx::ObjManager& om) {
    this->BaseType::RecursiveReset(om);
    om.RecursiveReset(this->data);
    om.RecursiveReset(this->c);
    om.RecursiveReset(this->c2);
    om.RecursiveReset(this->c3);
}
void B::SetDefaultValue(::xx::ObjManager& om) {
    this->BaseType::SetDefaultValue(om);
    om.SetDefaultValue(this->data);
    om.SetDefaultValue(this->c);
    om.SetDefaultValue(this->c2);
    om.SetDefaultValue(this->c3);
}
