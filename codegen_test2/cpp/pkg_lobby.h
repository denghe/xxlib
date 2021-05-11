#pragma once
#include "pkg_generic.h"
#include "pkg_lobby.h.inc"
struct CodeGen_pkg_lobby {
	inline static const ::std::string md5 = "#*MD5<06f6182325a53dbd958ca0f30bf111e4>*#";
    static void Register();
    CodeGen_pkg_lobby() { Register(); }
};
inline CodeGen_pkg_lobby __CodeGen_pkg_lobby;
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
#include "pkg_lobby_.h.inc"
