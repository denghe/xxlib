#include "foo.h"
#include "foo.cpp.inc"
void CodeGen_foo::Register() {
	::xx::ObjManager::Register<::foo>();
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
