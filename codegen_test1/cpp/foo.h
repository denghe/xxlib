#pragma once
#include "xx_obj.h"
#include "foo.h.inc"
struct CodeGen_foo {
	inline static const ::std::string md5 = "#*MD5<1e1cb9c0fb46dfb1da1e5ddcfa201531>*#";
    static void Register();
    CodeGen_foo() { Register(); }
};
inline CodeGen_foo __CodeGen_foo;
struct foo;
struct bar;
namespace xx {
    template<> struct TypeId<::foo> { static const uint16_t value = 16; };
    template<> struct TypeId<::bar> { static const uint16_t value = 22; };
}

struct foo2 {
    XX_OBJ_STRUCT_H(foo2)
    int32_t id = 0;
    ::std::string name;
};
struct foo : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(foo, ::xx::ObjBase)
    int32_t id = 0;
    ::std::string name;
};
struct bar : ::foo {
    XX_OBJ_OBJECT_H(bar, ::foo)
};
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::foo2)
}
#include "foo_.h.inc"
