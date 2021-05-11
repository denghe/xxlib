#pragma once
#include "pkg_generic.h"
#include "pkg_lobby.h.inc"
struct CodeGen_pkg_lobby {
	inline static const ::std::string md5 = "#*MD5<69b53f7150a759a7e03b4323f932f4a9>*#";
    static void Register();
    CodeGen_pkg_lobby() { Register(); }
};
inline CodeGen_pkg_lobby __CodeGen_pkg_lobby;
namespace Lobby_Client::Response::Auth { struct Online; }
namespace Lobby_Client::Response::Auth { struct Restore; }
namespace Client_Lobby::Request { struct Auth; }
namespace xx {
    template<> struct TypeId<::Lobby_Client::Response::Auth::Online> { static const uint16_t value = 11; };
    template<> struct TypeId<::Lobby_Client::Response::Auth::Restore> { static const uint16_t value = 12; };
    template<> struct TypeId<::Client_Lobby::Request::Auth> { static const uint16_t value = 10; };
}

namespace Lobby_Client::Response::Auth {
    struct Online : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Online, ::xx::ObjBase)
        using IsSimpleType_v = Online;
        int32_t accountId = 0;
    };
}
namespace Lobby_Client::Response::Auth {
    struct Restore : ::Lobby_Client::Response::Auth::Online {
        XX_OBJ_OBJECT_H(Restore, ::Lobby_Client::Response::Auth::Online)
        using IsSimpleType_v = Restore;
        int32_t serviceId = 0;
    };
}
namespace Client_Lobby::Request {
    struct Auth : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Auth, ::xx::ObjBase)
        using IsSimpleType_v = Auth;
        ::std::string username;
        ::std::string password;
    };
}
#include "pkg_lobby_.h.inc"
