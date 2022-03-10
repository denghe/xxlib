#pragma once
#include <pkg_generic.h>
#include <pkg_game_lobby.h.inc>
struct CodeGen_pkg_game_lobby {
	inline static const ::std::string md5 = "#*MD5<03fea6a7cfb9e0ca6a3fc9386864a402>*#";
    static void Register();
    CodeGen_pkg_game_lobby() { Register(); }
};
inline CodeGen_pkg_game_lobby __CodeGen_pkg_game_lobby;
namespace Lobby_Game { struct PlayerEnter; }
namespace Lobby_Game { struct PlayerLeave; }
namespace Game_Lobby { struct Register; }
namespace Game_Lobby { struct PlayerLeave; }
namespace xx {
    template<> struct TypeId<::Lobby_Game::PlayerEnter> { static const uint16_t value = 210; };
    template<> struct TypeId<::Lobby_Game::PlayerLeave> { static const uint16_t value = 211; };
    template<> struct TypeId<::Game_Lobby::Register> { static const uint16_t value = 200; };
    template<> struct TypeId<::Game_Lobby::PlayerLeave> { static const uint16_t value = 201; };
}

namespace Lobby_Game {
    // 请求：玩家进入. 成功返回 Generic.Success. 错误或异常 返回 Generic.Error
    struct PlayerEnter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerEnter, ::xx::ObjBase)
        using IsSimpleType_v = PlayerEnter;
        // 网关标识
        uint32_t gatewayId = 0;
        // 网关连接标识
        uint32_t clientId = 0;
        // 进入哪个游戏
        int32_t gameId = 0;
        // 玩家信息顺便传递，避免查询 db
        ::Generic::PlayerInfo playerInfo;
        static void WriteTo(xx::Data& d, uint32_t const& gatewayId, uint32_t const& clientId, int32_t const& gameId, ::Generic::PlayerInfo const& playerInfo);
    };
}
namespace Lobby_Game {
    // 请求：强制玩家立刻退出游戏( 断线, 结存, 清除上下文 ). 操作完毕后返回 Generic.Success. 错误或异常 返回 Generic.Error
    struct PlayerLeave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerLeave, ::xx::ObjBase)
        using IsSimpleType_v = PlayerLeave;
        // 玩家标识
        int32_t accountId = 0;
        // 操作原因
        ::std::string reason;
        static void WriteTo(xx::Data& d, int32_t const& accountId, std::string_view const& reason);
    };
}
namespace Game_Lobby {
    // 请求：注册游戏. 一个服务可注册多个游戏. 注册后, 已在线客户端将收到开服通知，成功返回 Generic.Success. 错误或异常 返回 Generic.Error
    struct Register : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Register, ::xx::ObjBase)
        using IsSimpleType_v = Register;
        uint32_t serviceId = 0;
        ::std::vector<::Generic::GameInfo> gameInfos;
        static void WriteTo(xx::Data& d, uint32_t const& serviceId, ::std::vector<::Generic::GameInfo> const& gameInfos);
    };
}
namespace Game_Lobby {
    // 推送：玩家已退出游戏
    struct PlayerLeave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerLeave, ::xx::ObjBase)
        using IsSimpleType_v = PlayerLeave;
        // 玩家标识
        int32_t accountId = 0;
        int32_t gameId = 0;
        static void WriteTo(xx::Data& d, int32_t const& accountId, int32_t const& gameId);
    };
}
#include <pkg_game_lobby_.h.inc>
