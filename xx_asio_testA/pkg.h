#pragma once
#include <xx_obj.h>
#include <pkg.h.inc>
struct CodeGen_pkg {
	inline static const ::std::string md5 = "#*MD5<8d32151f5156ded6c9e41dea774711c0>*#";
    static void Register();
    CodeGen_pkg() { Register(); }
};
inline CodeGen_pkg __CodeGen_pkg;
struct Ping;
struct Pong;
namespace xx {
    template<> struct TypeId<::Ping> { static const uint16_t value = 2062; };
    template<> struct TypeId<::Pong> { static const uint16_t value = 1283; };
}

struct Ping : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(Ping, ::xx::ObjBase)
    using IsSimpleType_v = Ping;
    int64_t ticks = 0;
    static void WriteTo(xx::Data& d, int64_t const& ticks);
};
struct Pong : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(Pong, ::xx::ObjBase)
    using IsSimpleType_v = Pong;
    int64_t ticks = 0;
    static void WriteTo(xx::Data& d, int64_t const& ticks);
};
#include <pkg_.h.inc>
