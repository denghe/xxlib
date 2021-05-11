#pragma once
#include "pkg_generic.h"
#include "pkg_lobby.h.inc"
struct CodeGen_pkg_lobby {
	inline static const ::std::string md5 = "#*MD5<2463ad580161c16fed5d7b8aba45b218>*#";
    static void Register();
    CodeGen_pkg_lobby() { Register(); }
};
inline CodeGen_pkg_lobby __CodeGen_pkg_lobby;
namespace Lobby_Client::Auth { struct Online; }
namespace Lobby_Client::Auth { struct Error; }
namespace Lobby_Client::Auth { struct Restore; }
namespace Client_Lobby { struct Auth; }
namespace xx {
    template<> struct TypeId<::Lobby_Client::Auth::Online> { static const uint16_t value = 12; };
    template<> struct TypeId<::Lobby_Client::Auth::Error> { static const uint16_t value = 11; };
    template<> struct TypeId<::Lobby_Client::Auth::Restore> { static const uint16_t value = 13; };
    template<> struct TypeId<::Client_Lobby::Auth> { static const uint16_t value = 10; };
}

namespace Lobby_Client::Auth {
    struct Online : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Online, ::xx::ObjBase)
        using IsSimpleType_v = Online;
        int32_t accountId = 0;
    };
}
namespace Lobby_Client::Auth {
    struct Error : ::Generic::Error {
        XX_OBJ_OBJECT_H(Error, ::Generic::Error)
        using IsSimpleType_v = Error;
    };
}
namespace Lobby_Client::Auth {
    struct Restore : ::Lobby_Client::Auth::Online {
        XX_OBJ_OBJECT_H(Restore, ::Lobby_Client::Auth::Online)
        using IsSimpleType_v = Restore;
        int32_t serviceId = 0;
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
