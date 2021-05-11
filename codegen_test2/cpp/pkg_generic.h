#pragma once
#include "xx_obj.h"
#include "pkg_generic.h.inc"
struct CodeGen_pkg_generic {
	inline static const ::std::string md5 = "#*MD5<73a86f606c1370ac143df00e31c7e076>*#";
    static void Register();
    CodeGen_pkg_generic() { Register(); }
};
inline CodeGen_pkg_generic __CodeGen_pkg_generic;
namespace Generic { struct Success; }
namespace Generic { struct Error; }
namespace xx {
    template<> struct TypeId<::Generic::Success> { static const uint16_t value = 1; };
    template<> struct TypeId<::Generic::Error> { static const uint16_t value = 2; };
}

namespace Generic {
    struct Success : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Success, ::xx::ObjBase)
        using IsSimpleType_v = Success;
    };
}
namespace Generic {
    struct Error : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Error, ::xx::ObjBase)
        using IsSimpleType_v = Error;
        int32_t errNumber = 0;
        ::std::string errMessage;
    };
}
#include "pkg_generic_.h.inc"
