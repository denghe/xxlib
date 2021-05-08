#pragma once
#include "xx_obj.h"
#include "pkg.h.inc"
struct CodeGen_pkg {
	inline static const ::std::string md5 = "#*MD5<01a4f94996362665c3f08071f8228e75>*#";
    static void Register();
    CodeGen_pkg() { Register(); }
};
inline CodeGen_pkg __CodeGen_pkg;
namespace Lobby_Client { struct AuthResult; }
namespace Client_Lobby { struct Auth; }
namespace xx {
    template<> struct TypeId<::Lobby_Client::AuthResult> { static const uint16_t value = 11; };
    template<> struct TypeId<::Client_Lobby::Auth> { static const uint16_t value = 10; };
}

namespace Lobby_Client {
    struct AuthResult : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(AuthResult, ::xx::ObjBase)
        using IsSimpleType_v = AuthResult;
        // -1: not found
        int32_t accountId = 0;
    };
}
namespace Client_Lobby {
    struct Auth : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Auth, ::xx::ObjBase)
        using IsSimpleType_v = Auth;
        ::std::string username;
        ::std::string password;
    };
}
#include "pkg_.h.inc"
