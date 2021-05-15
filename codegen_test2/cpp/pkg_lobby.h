#pragma once
#include "pkg_generic.h"
#include "pkg_lobby.h.inc"
struct CodeGen_pkg_lobby {
	inline static const ::std::string md5 = "#*MD5<42423a0268b8cb8041f36180c33342ad>*#";
    static void Register();
    CodeGen_pkg_lobby() { Register(); }
};
inline CodeGen_pkg_lobby __CodeGen_pkg_lobby;
namespace Lobby_Client::Auth { struct Error; }
namespace Lobby_Client::Auth { struct Success; }
namespace Client_Lobby { struct Auth; }
namespace xx {
    template<> struct TypeId<::Lobby_Client::Auth::Error> { static const uint16_t value = 11; };
    template<> struct TypeId<::Lobby_Client::Auth::Success> { static const uint16_t value = 12; };
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
    // 登录出错
    struct Error : ::Generic::Error {
        XX_OBJ_OBJECT_H(Error, ::Generic::Error)
        using IsSimpleType_v = Error;
    };
}
namespace Lobby_Client::Auth {
    // 登录成功 新上线 或 恢复
    struct Success : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Success, ::xx::ObjBase)
        using IsSimpleType_v = Success;
        int32_t accountId = 0;
        ::std::string nickname;
        double coin = 0;
        ::std::vector<::Lobby_Client::Game> games;
        // 要回到的游戏id( 小于0: 在大厅 )
        int32_t gameId = 0;
        // 要等待 open 的服务id( 小于0: 新上线  等于0: 大厅顶下线  大于0: 游戏服务id )
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
