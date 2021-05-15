#pragma once
#include "pkg_generic.h"
#include "pkg_lobby_client.h.inc"
struct CodeGen_pkg_lobby_client {
	inline static const ::std::string md5 = "#*MD5<aeffcf6b8032db7ef4b928939dbb2b6d>*#";
    static void Register();
    CodeGen_pkg_lobby_client() { Register(); }
};
inline CodeGen_pkg_lobby_client __CodeGen_pkg_lobby_client;
namespace Lobby_Client { struct PlayerContext; }
namespace Lobby_Client { struct EnterGameSuccess; }
namespace Lobby_Client { struct GameOpen; }
namespace Lobby_Client { struct GameClose; }
namespace Client_Lobby { struct Auth; }
namespace Client_Lobby { struct EnterGame; }
namespace xx {
    template<> struct TypeId<::Lobby_Client::PlayerContext> { static const uint16_t value = 20; };
    template<> struct TypeId<::Lobby_Client::EnterGameSuccess> { static const uint16_t value = 21; };
    template<> struct TypeId<::Lobby_Client::GameOpen> { static const uint16_t value = 30; };
    template<> struct TypeId<::Lobby_Client::GameClose> { static const uint16_t value = 31; };
    template<> struct TypeId<::Client_Lobby::Auth> { static const uint16_t value = 10; };
    template<> struct TypeId<::Client_Lobby::EnterGame> { static const uint16_t value = 11; };
}

namespace Lobby_Client {
    // 回应：登录成功( 新上线/顶下线 或 恢复游戏 ). 通常接下来将推送 GameOpen
    struct PlayerContext : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerContext, ::xx::ObjBase)
        using IsSimpleType_v = PlayerContext;
        // 自己( 当前账号 )的基础信息
        ::Generic::PlayerInfo self;
        // 当前所在游戏id( 小于0: 在大厅 )
        int32_t gameId = 0;
        // 要等待 open 的服务id( 小于0: 新上线 不用等  等于0: 大厅顶下线 不用等  大于0: 游戏服务id )
        int32_t serviceId = 0;
    };
}
namespace Lobby_Client {
    // 回应：进游戏成功
    struct EnterGameSuccess : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(EnterGameSuccess, ::xx::ObjBase)
        using IsSimpleType_v = EnterGameSuccess;
        // 要等待 open 的服务id
        int32_t serviceId = 0;
    };
}
namespace Lobby_Client {
    // 推送: 游戏开服
    struct GameOpen : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GameOpen, ::xx::ObjBase)
        using IsSimpleType_v = GameOpen;
        // 游戏信息列表
        ::std::vector<::Generic::GameInfo> gameInfos;
    };
}
namespace Lobby_Client {
    // 推送: 游戏关服
    struct GameClose : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GameClose, ::xx::ObjBase)
        using IsSimpleType_v = GameClose;
        // 游戏标识列表
        ::std::vector<int32_t> gameIds;
    };
}
namespace Client_Lobby {
    // 请求：登录/校验. 成功返回 Lobby_Client.PlayerContext ( 也可能返回排队之类 ), 出错返回 Generic.Error
    struct Auth : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Auth, ::xx::ObjBase)
        using IsSimpleType_v = Auth;
        ::std::string username;
        ::std::string password;
    };
}
namespace Client_Lobby {
    // 请求：进入游戏. 成功返回 Lobby_Client.EnterGameSuccess, 出错返回 Generic.Error
    struct EnterGame : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(EnterGame, ::xx::ObjBase)
        using IsSimpleType_v = EnterGame;
        int32_t gameId = 0;
    };
}
#include "pkg_lobby_client_.h.inc"
