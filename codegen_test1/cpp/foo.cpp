﻿#include "foo.h"
#include "foo.cpp.inc"
void CodeGen_foo::Register() {
	::xx::ObjManager::Register<::foo>();
	::xx::ObjManager::Register<::FishBase>();
	::xx::ObjManager::Register<::bar>();
	::xx::ObjManager::Register<::FishWithChilds>();
}
namespace xx {
	void ObjFuncs<::foo2, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::foo2 const& in) {
        om.Write(d, in.id);
        om.Write(d, in.d);
    }
	void ObjFuncs<::foo2, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::foo2 const& in) {
        om.Write<false>(d, in.id);
        om.Write<false>(d, in.d);
    }
	int ObjFuncs<::foo2, void>::Read(::xx::ObjManager& om, ::xx::Data_r& d, ::foo2& out) {
        if (int r = om.Read(d, out.id)) return r;
        if (int r = om.Read(d, out.d)) return r;
        return 0;
    }
	void ObjFuncs<::foo2, void>::Append(ObjManager &om, std::string& s, ::foo2 const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::foo2, void>::AppendCore(ObjManager &om, std::string& s, ::foo2 const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"id\":", in.id); 
        om.Append(s, ",\"d\":", in.d);
#endif
    }
    void ObjFuncs<::foo2>::Clone(::xx::ObjManager& om, ::foo2 const& in, ::foo2 &out) {
        om.Clone_(in.id, out.id);
        om.Clone_(in.d, out.d);
    }
    int ObjFuncs<::foo2>::RecursiveCheck(::xx::ObjManager& om, ::foo2 const& in) {
        if (int r = om.RecursiveCheck(in.id)) return r;
        if (int r = om.RecursiveCheck(in.d)) return r;
        return 0;
    }
    void ObjFuncs<::foo2>::RecursiveReset(::xx::ObjManager& om, ::foo2& in) {
        om.RecursiveReset(in.id);
        om.RecursiveReset(in.d);
    }
    void ObjFuncs<::foo2>::SetDefaultValue(::xx::ObjManager& om, ::foo2& in) {
        in.id = 0;
        om.SetDefaultValue(in.d);
    }
}
void foo::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    om.Write(d, this->id);
    om.Write(d, this->name);
}
int foo::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = om.Read(d, this->id)) return r;
    if (int r = om.Read(d, this->name)) return r;
    return 0;
}
void foo::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":16");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void foo::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"id\":", this->id);
    om.Append(s, ",\"name\":", this->name);
#endif
}
void foo::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::foo*)tar;
    om.Clone_(this->id, out->id);
    om.Clone_(this->name, out->name);
}
int foo::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->id)) return r;
    if (int r = om.RecursiveCheck(this->name)) return r;
    return 0;
}
void foo::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->id);
    om.RecursiveReset(this->name);
}
void foo::SetDefaultValue(::xx::ObjManager& om) {
    this->id = 0;
    om.SetDefaultValue(this->name);
}
void FishBase::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    om.Write(d, this->cfgId);
    om.Write(d, this->id);
    om.Write(d, this->x);
    om.Write(d, this->y);
    om.Write(d, this->a);
    om.Write(d, this->r);
    om.Write(d, this->coin);
}
int FishBase::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = om.Read(d, this->cfgId)) return r;
    if (int r = om.Read(d, this->id)) return r;
    if (int r = om.Read(d, this->x)) return r;
    if (int r = om.Read(d, this->y)) return r;
    if (int r = om.Read(d, this->a)) return r;
    if (int r = om.Read(d, this->r)) return r;
    if (int r = om.Read(d, this->coin)) return r;
    return 0;
}
void FishBase::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":30");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void FishBase::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"cfgId\":", this->cfgId);
    om.Append(s, ",\"id\":", this->id);
    om.Append(s, ",\"x\":", this->x);
    om.Append(s, ",\"y\":", this->y);
    om.Append(s, ",\"a\":", this->a);
    om.Append(s, ",\"r\":", this->r);
    om.Append(s, ",\"coin\":", this->coin);
#endif
}
void FishBase::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::FishBase*)tar;
    om.Clone_(this->cfgId, out->cfgId);
    om.Clone_(this->id, out->id);
    om.Clone_(this->x, out->x);
    om.Clone_(this->y, out->y);
    om.Clone_(this->a, out->a);
    om.Clone_(this->r, out->r);
    om.Clone_(this->coin, out->coin);
}
int FishBase::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->cfgId)) return r;
    if (int r = om.RecursiveCheck(this->id)) return r;
    if (int r = om.RecursiveCheck(this->x)) return r;
    if (int r = om.RecursiveCheck(this->y)) return r;
    if (int r = om.RecursiveCheck(this->a)) return r;
    if (int r = om.RecursiveCheck(this->r)) return r;
    if (int r = om.RecursiveCheck(this->coin)) return r;
    return 0;
}
void FishBase::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->cfgId);
    om.RecursiveReset(this->id);
    om.RecursiveReset(this->x);
    om.RecursiveReset(this->y);
    om.RecursiveReset(this->a);
    om.RecursiveReset(this->r);
    om.RecursiveReset(this->coin);
}
void FishBase::SetDefaultValue(::xx::ObjManager& om) {
    this->cfgId = 0;
    this->id = 0;
    this->x = 0.0f;
    this->y = 0.0f;
    this->a = 0.0f;
    this->r = 0.0f;
    this->coin = 0;
}
void bar::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    this->BaseType::Write(om, d);
}
int bar::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = this->BaseType::Read(om, d)) return r;
    return 0;
}
void bar::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":22");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void bar::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    this->BaseType::AppendCore(om, s);
#endif
}
void bar::Clone(::xx::ObjManager& om, void* const &tar) const {
    this->BaseType::Clone(om, tar);
}
int bar::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = this->BaseType::RecursiveCheck(om)) return r;
    return 0;
}
void bar::RecursiveReset(::xx::ObjManager& om) {
    this->BaseType::RecursiveReset(om);
}
void bar::SetDefaultValue(::xx::ObjManager& om) {
    this->BaseType::SetDefaultValue(om);
}
void FishWithChilds::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    this->BaseType::Write(om, d);
    om.Write(d, this->childs);
}
int FishWithChilds::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = this->BaseType::Read(om, d)) return r;
    if (int r = om.Read(d, this->childs)) return r;
    return 0;
}
void FishWithChilds::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":31");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void FishWithChilds::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    this->BaseType::AppendCore(om, s);
    om.Append(s, ",\"childs\":", this->childs);
#endif
}
void FishWithChilds::Clone(::xx::ObjManager& om, void* const &tar) const {
    this->BaseType::Clone(om, tar);
    auto out = (::FishWithChilds*)tar;
    om.Clone_(this->childs, out->childs);
}
int FishWithChilds::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = this->BaseType::RecursiveCheck(om)) return r;
    if (int r = om.RecursiveCheck(this->childs)) return r;
    return 0;
}
void FishWithChilds::RecursiveReset(::xx::ObjManager& om) {
    this->BaseType::RecursiveReset(om);
    om.RecursiveReset(this->childs);
}
void FishWithChilds::SetDefaultValue(::xx::ObjManager& om) {
    this->BaseType::SetDefaultValue(om);
    om.SetDefaultValue(this->childs);
}
