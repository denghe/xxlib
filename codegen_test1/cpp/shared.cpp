#include "shared.h"
#include "shared.cpp.inc"
void CodeGen_shared::Register() {
	::xx::ObjManager::Register<::A>();
	::xx::ObjManager::Register<::B>();
}
namespace xx {
	void ObjFuncs<::C, void>::Write(::xx::ObjManager& om, ::xx::Data& d, ::C const& in) {
        om.Write(d, in.x);
        om.Write(d, in.y);
        om.Write(d, in.targets);
    }
	void ObjFuncs<::C, void>::WriteFast(::xx::ObjManager& om, ::xx::Data& d, ::C const& in) {
        om.Write<false>(d, in.x);
        om.Write<false>(d, in.y);
        om.Write<false>(d, in.targets);
    }
	int ObjFuncs<::C, void>::Read(::xx::ObjManager& om, ::xx::Data& d, ::C& out) {
        if (int r = om.Read(d, out.x)) return r;
        if (int r = om.Read(d, out.y)) return r;
        if (int r = om.Read(d, out.targets)) return r;
        return 0;
    }
	void ObjFuncs<::C, void>::Append(ObjManager &om, std::string& s, ::C const& in) {
#ifndef XX_DISABLE_APPEND
        s.push_back('{');
        AppendCore(om, s, in);
        s.push_back('}');
#endif
    }
	void ObjFuncs<::C, void>::AppendCore(ObjManager &om, std::string& s, ::C const& in) {
#ifndef XX_DISABLE_APPEND
        om.Append(s, "\"x\":", in.x); 
        om.Append(s, ",\"y\":", in.y);
        om.Append(s, ",\"targets\":", in.targets);
#endif
    }
    void ObjFuncs<::C>::Clone(::xx::ObjManager& om, ::C const& in, ::C &out) {
        om.Clone_(in.x, out.x);
        om.Clone_(in.y, out.y);
        om.Clone_(in.targets, out.targets);
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
void A::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    auto bak = d.WriteJump(sizeof(uint32_t));
    om.Write(d, this->id);
    om.Write(d, this->nick);
    om.Write(d, this->parent);
    om.Write(d, this->children);
    d.WriteFixedAt(bak, (uint32_t)(d.len - bak));
}
int A::Read(::xx::ObjManager& om, ::xx::Data& d) {
    uint32_t siz;
    if (int r = d.ReadFixed(siz)) return r;
    auto endOffset = d.offset - sizeof(siz) + siz;

    if (d.offset >= endOffset) this->id = 0;
    else if (int r = om.Read(d, this->id)) return r;
    if (d.offset >= endOffset) om.SetDefaultValue(this->nick);
    else if (int r = om.Read(d, this->nick)) return r;
    if (d.offset >= endOffset) om.SetDefaultValue(this->parent);
    else if (int r = om.Read(d, this->parent)) return r;
    if (d.offset >= endOffset) om.SetDefaultValue(this->children);
    else if (int r = om.Read(d, this->children)) return r;

    if (d.offset > endOffset) return __LINE__;
    else d.offset = endOffset;
    return 0;
}
void A::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":1");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void A::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, ",\"id\":", this->id);
    ::xx::Append(s, ",\"nick\":", this->nick);
    ::xx::Append(s, ",\"parent\":", this->parent);
    ::xx::Append(s, ",\"children\":", this->children);
#endif
}
void A::Clone(::xx::ObjManager& om, void* const &tar) const {
    auto out = (::A*)tar;
    om.Clone_(this->id, out->id);
    om.Clone_(this->nick, out->nick);
    om.Clone_(this->parent, out->parent);
    om.Clone_(this->children, out->children);
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
void B::Write(::xx::ObjManager& om, ::xx::Data& d) const {
    this->BaseType::Write(om, d);
    om.Write(d, this->data);
    om.Write(d, this->c);
    om.Write(d, this->c2);
    om.Write(d, this->c3);
}
int B::Read(::xx::ObjManager& om, ::xx::Data& d) {
    if (int r = this->BaseType::Read(om, d)) return r;
    if (int r = om.Read(d, this->data)) return r;
    if (int r = om.Read(d, this->c)) return r;
    if (int r = om.Read(d, this->c2)) return r;
    if (int r = om.Read(d, this->c3)) return r;
    return 0;
}
void B::Append(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    ::xx::Append(s, "{\"__typeId__\":2");
    this->AppendCore(om, s);
    s.push_back('}');
#endif
}
void B::AppendCore(::xx::ObjManager& om, std::string& s) const {
#ifndef XX_DISABLE_APPEND
    this->BaseType::AppendCore(om, s);
    ::xx::Append(s, ",\"data\":", this->data);
    ::xx::Append(s, ",\"c\":", this->c);
    ::xx::Append(s, ",\"c2\":", this->c2);
    ::xx::Append(s, ",\"c3\":", this->c3);
#endif
}
void B::Clone(::xx::ObjManager& om, void* const &tar) const {
    this->BaseType::Clone(om, tar);
    auto out = (::B*)tar;
    om.Clone_(this->data, out->data);
    om.Clone_(this->c, out->c);
    om.Clone_(this->c2, out->c2);
    om.Clone_(this->c3, out->c3);
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
