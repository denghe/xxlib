#include "foo.h"
#include "foo.cpp.inc"
void CodeGen_foo::Register() {
	::xx::ObjManager::Register<::foo>();
	::xx::ObjManager::Register<::bar>();
}
namespace xx {
	void ObjFuncs<::foo2, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::foo2 const& in) {
        om.Write(d, in.id);
        om.Write(d, in.name);
    }
	int ObjFuncs<::foo2, void>::Read(::xx::ObjManager& om, ::xx::Data& d, ::foo2& out) {
        if (int r = om.Read(d, out.id)) return r;
        if (int r = om.Read(d, out.name)) return r;
        return 0;
    }
	void ObjFuncs<::foo2, void>::Append(ObjManager &om, ::foo2 const& in) {
#ifndef XX_DISABLE_APPEND
        om.str->push_back('{');
        AppendCore(om, in);
        om.str->push_back('}');
#endif
    }
	void ObjFuncs<::foo2, void>::AppendCore(ObjManager &om, ::foo2 const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append("\"id\":", in.id); 
        om.Append(",\"name\":", in.name);
#endif
    }
    void ObjFuncs<::foo2>::Clone(::xx::ObjManager& om, ::foo2 const& in, ::foo2 &out) {
        om.Clone_(in.id, out.id);
        om.Clone_(in.name, out.name);
    }
    int ObjFuncs<::foo2>::RecursiveCheck(::xx::ObjManager& om, ::foo2 const& in) {
        if (int r = om.RecursiveCheck(in.id)) return r;
        if (int r = om.RecursiveCheck(in.name)) return r;
        return 0;
    }
    void ObjFuncs<::foo2>::RecursiveReset(::xx::ObjManager& om, ::foo2& in) {
        om.RecursiveReset(in.id);
        om.RecursiveReset(in.name);
    }
    void ObjFuncs<::foo2>::SetDefaultValue(::xx::ObjManager& om, ::foo2& in) {
        in.id = 0;
        om.SetDefaultValue(in.name);
    }
}
void foo::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    om.Write(d, this->id);
    om.Write(d, this->name);
}
int foo::Read(::xx::ObjManager& om, ::xx::Data& d) {
    if (int r = om.Read(d, this->id)) return r;
    if (int r = om.Read(d, this->name)) return r;
    return 0;
}
void foo::Append(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append("{\"__typeId__\":16");
    this->AppendCore(om);
    om.str->push_back('}');
#endif
}
void foo::AppendCore(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append(",\"id\":", this->id);
    om.Append(",\"name\":", this->name);
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
void bar::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    this->BaseType::Write(om, d);
}
int bar::Read(::xx::ObjManager& om, ::xx::Data& d) {
    if (int r = this->BaseType::Read(om, d)) return r;
    return 0;
}
void bar::Append(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    om.Append("{\"__typeId__\":22");
    this->AppendCore(om);
    om.str->push_back('}');
#endif
}
void bar::AppendCore(::xx::ObjManager& om) const {
#ifndef XX_DISABLE_APPEND
    this->BaseType::AppendCore(om);
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
