#include <pkg.h>
#include <pkg.cpp.inc>
void CodeGen_pkg::Register() {
	::xx::ObjManager::Register<::Ping>();
	::xx::ObjManager::Register<::Pong>();
}
void Ping::WriteTo(xx::Data& d, int64_t const& ticks) {
    d.Write(xx::TypeId_v<Ping>);
    d.Write(ticks);
}
void Ping::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    d.Write(this->ticks);
}
int Ping::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = d.Read(this->ticks)) return r;
    return 0;
}
void Ping::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":2062");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void Ping::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"ticks\":", this->ticks);
#endif
}
void Ping::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::Ping*)tar;
    om.Clone_(this->ticks, out->ticks);
}
int Ping::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->ticks)) return r;
    return 0;
}
void Ping::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->ticks);
}
void Ping::SetDefaultValue(::xx::ObjManager& om) {
    this->ticks = 0;
}
void Pong::WriteTo(xx::Data& d, int64_t const& ticks) {
    d.Write(xx::TypeId_v<Pong>);
    d.Write(ticks);
}
void Pong::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    d.Write(this->ticks);
}
int Pong::Read(::xx::ObjManager& om, ::xx::Data_r& d) {
    if (int r = d.Read(this->ticks)) return r;
    return 0;
}
void Pong::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":1283");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void Pong::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    om.Append(s, ",\"ticks\":", this->ticks);
#endif
}
void Pong::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::Pong*)tar;
    om.Clone_(this->ticks, out->ticks);
}
int Pong::RecursiveCheck(::xx::ObjManager& om) const {
    if (int r = om.RecursiveCheck(this->ticks)) return r;
    return 0;
}
void Pong::RecursiveReset(::xx::ObjManager& om) {
    om.RecursiveReset(this->ticks);
}
void Pong::SetDefaultValue(::xx::ObjManager& om) {
    this->ticks = 0;
}
