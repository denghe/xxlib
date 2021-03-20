#pragma once
#include "xx_obj.h"
#include "shared.h.inc"
struct CodeGen_shared {
	inline static const ::std::string md5 = "#*MD5<20cc599a2933d0e16e23ea4de6548220>*#";
    static void Register();
    CodeGen_shared() { Register(); }
};
inline CodeGen_shared __CodeGen_shared;
struct A;
struct B;
namespace xx {
    template<> struct TypeId<::A> { static const uint16_t value = 1; };
    template<> struct TypeId<::B> { static const uint16_t value = 2; };
}

// 我怕擦
struct C {
    XX_OBJ_STRUCT_H(C)
    float x = 0.0f;
    float y = 0.0f;
    ::std::vector<::xx::Weak<::A>> targets;
};
struct A : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(A, ::xx::ObjBase)
#include "shared_A.inc"
    int32_t id = 0;
    ::std::optional<::std::string> nick;
    ::xx::Weak<::A> parent;
    ::std::vector<::xx::Shared<::A>> children;
};
struct B : ::A {
    XX_OBJ_OBJECT_H(B, ::A)
#include "shared_B.inc"
    ::xx::Data data;
    ::C c;
    ::std::optional<::C> c2;
    ::std::vector<::std::optional<::C>> c3;
};
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::C)
}
#include "shared_.h.inc"
