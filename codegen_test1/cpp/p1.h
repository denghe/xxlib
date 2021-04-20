#pragma once
#include "shared.h"
#include "p1.h.inc"
struct CodeGen_p1 {
	inline static const ::std::string md5 = "#*MD5<47b5624491a9efe7d6523945ff990d26>*#";
    static void Register();
    CodeGen_p1() { Register(); }
};
inline CodeGen_p1 __CodeGen_p1;
namespace p1 { struct p1c1; }
namespace xx {
    template<> struct TypeId<::p1::p1c1> { static const uint16_t value = 10; };
}

namespace p1 {
    // 我草
    struct p1c1 : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(p1c1, ::xx::ObjBase)
#include "p1_p1p1c1.inc"
        ::A a;
    };
}
#include "p1_.h.inc"
