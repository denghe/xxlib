#pragma once
#include "xx_obj.h"
#include "foo.h.inc"
struct CodeGen_foo {
	inline static const ::std::string md5 = "#*MD5<7a578c9d53f63f4d9aec554d5688774f>*#";
    static void Register();
    CodeGen_foo() { Register(); }
};
inline CodeGen_foo __CodeGen_foo;
struct foo;
struct FishBase;
struct bar;
struct FishWithChilds;
namespace xx {
    template<> struct TypeId<::foo> { static const uint16_t value = 16; };
    template<> struct TypeId<::FishBase> { static const uint16_t value = 30; };
    template<> struct TypeId<::bar> { static const uint16_t value = 22; };
    template<> struct TypeId<::FishWithChilds> { static const uint16_t value = 31; };
}

struct foo2 {
    XX_OBJ_STRUCT_H(foo2)
    using IsSimpleType_v = foo2;
    int32_t id = 0;
    ::std::string name;
};
struct foo : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(foo, ::xx::ObjBase)
    using IsSimpleType_v = foo;
    int32_t id = 0;
    ::std::string name;
};
struct FishBase : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(FishBase, ::xx::ObjBase)
    using IsSimpleType_v = FishBase;
    int32_t cfgId = 0;
    int32_t id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float a = 0.0f;
    float r = 0.0f;
    int64_t coin = 0;
};
struct bar : ::foo {
    XX_OBJ_OBJECT_H(bar, ::foo)
    using IsSimpleType_v = bar;
};
struct FishWithChilds : ::FishBase {
    XX_OBJ_OBJECT_H(FishWithChilds, ::FishBase)
    ::std::vector<::xx::Shared<::FishBase>> childs;
};
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::foo2)
}
#include "foo_.h.inc"
