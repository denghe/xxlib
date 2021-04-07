#pragma once
#include "xx_obj.h"
#include "foo.h.inc"
struct CodeGen_foo {
	inline static const ::std::string md5 = "#*MD5<1bab455bd56bb529213e64947d949ea1>*#";
    static void Register();
    CodeGen_foo() { Register(); }
};
inline CodeGen_foo __CodeGen_foo;
struct foo;
namespace xx {
    template<> struct TypeId<::foo> { static const uint16_t value = 16; };
}

struct foo : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(foo, ::xx::ObjBase)
    int32_t id = 0;
    ::std::string name;
};
#include "foo_.h.inc"
