#pragma once
#include "pkg_generic.h"
#include "pkg_lobby.h.inc"
struct CodeGen_pkg_lobby {
	inline static const ::std::string md5 = "#*MD5<f19308bfaa5bcea54e81ae149a434be4>*#";
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

namespace Lobby_Client {
    // 游戏信息
    struct Game {
        XX_OBJ_STRUCT_H(Game)
        using IsSimpleType_v = Game;
        int32_t gameId = 0;
    };
}
namespace Lobby_Client::Auth {
    // 登录成功 上线( 不在游戏中的时候，即便是断线重连也下发这个包 )
    struct Online : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Online, ::xx::ObjBase)
        using IsSimpleType_v = Online;
        int32_t accountId = 0;
        ::std::string nickname;
        double coin = 0;
        ::std::vector<::Lobby_Client::Game> games;
    };
}
namespace Lobby_Client::Auth {
    // 登录出错
    struct Error : ::Generic::Error {
        XX_OBJ_OBJECT_H(Error, ::Generic::Error)
        using IsSimpleType_v = Error;
    };
}
namespace Lobby_Client::Auth {
    // 登录成功 恢复( 断线重连, 回到游戏 )
    struct Restore : ::Lobby_Client::Auth::Online {
        XX_OBJ_OBJECT_H(Restore, ::Lobby_Client::Auth::Online)
        using IsSimpleType_v = Restore;
        // 要回到的游戏id( 界面切换、预加载的凭据 )
        int32_t gameId = 0;
        // 要等待 open 的服务id( 等待网络就绪，和游戏服通信的地址 )
        int32_t serviceId = 0;
    };
}
namespace Client_Lobby {
    // 登录/校验
    struct Auth : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Auth, ::xx::ObjBase)
        using IsSimpleType_v = Auth;
        ::std::string username;
        ::std::string password;
    };
}
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::Lobby_Client::Game)
}
#include "pkg_lobby_.h.inc"
