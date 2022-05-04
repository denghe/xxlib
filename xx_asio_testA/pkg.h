#pragma once
#include <xx_obj.h>
#include <pkg.h.inc>
struct CodeGen_pkg {
	inline static const ::std::string md5 = "#*MD5<395150892adf4924cdc0a3255a56aa50>*#";
    static void Register();
    CodeGen_pkg() { Register(); }
};
inline CodeGen_pkg __CodeGen_pkg;
struct Ping;
struct Pong;
namespace Game1_Lobby { struct PlayerLeave; }
namespace Lobby_Game1 { struct PlayerEnter; }
namespace All_Db { struct GetPlayerId; }
namespace All_Db { struct GetPlayerInfo; }
namespace Generic { struct Success; }
namespace Generic { struct Error; }
namespace Generic { struct PlayerInfo; }
namespace xx {
    template<> struct TypeId<::Ping> { static const uint16_t value = 2062; };
    template<> struct TypeId<::Pong> { static const uint16_t value = 1283; };
    template<> struct TypeId<::Game1_Lobby::PlayerLeave> { static const uint16_t value = 401; };
    template<> struct TypeId<::Lobby_Game1::PlayerEnter> { static const uint16_t value = 301; };
    template<> struct TypeId<::All_Db::GetPlayerId> { static const uint16_t value = 201; };
    template<> struct TypeId<::All_Db::GetPlayerInfo> { static const uint16_t value = 202; };
    template<> struct TypeId<::Generic::Success> { static const uint16_t value = 10; };
    template<> struct TypeId<::Generic::Error> { static const uint16_t value = 11; };
    template<> struct TypeId<::Generic::PlayerInfo> { static const uint16_t value = 12; };
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
namespace Game1_Lobby {
    // return Generic.Error || Generic.Success{}
    struct PlayerLeave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerLeave, ::xx::ObjBase)
        using IsSimpleType_v = PlayerLeave;
        int64_t playerId = 0;
        static void WriteTo(xx::Data& d, int64_t const& playerId);
    };
}
namespace Lobby_Game1 {
    // return Generic.Error || Generic.Success{}
    struct PlayerEnter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerEnter, ::xx::ObjBase)
        using IsSimpleType_v = PlayerEnter;
        uint32_t gatewayId = 0;
        uint32_t clientId = 0;
        int64_t playerId = 0;
        static void WriteTo(xx::Data& d, uint32_t const& gatewayId, uint32_t const& clientId, int64_t const& playerId);
    };
}
namespace All_Db {
    // return Generic.Error || Generic.Success{ value = id }
    struct GetPlayerId : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetPlayerId, ::xx::ObjBase)
        using IsSimpleType_v = GetPlayerId;
        ::std::string username;
        ::std::string password;
        static void WriteTo(xx::Data& d, std::string_view const& username, std::string_view const& password);
    };
}
namespace All_Db {
    // return Generic.Error || Generic.PlayerInfo
    struct GetPlayerInfo : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetPlayerInfo, ::xx::ObjBase)
        using IsSimpleType_v = GetPlayerInfo;
        int64_t id = 0;
        static void WriteTo(xx::Data& d, int64_t const& id);
    };
}
namespace Generic {
    struct Success : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Success, ::xx::ObjBase)
        using IsSimpleType_v = Success;
        int64_t value = 0;
        static void WriteTo(xx::Data& d, int64_t const& value);
    };
}
namespace Generic {
    struct Error : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Error, ::xx::ObjBase)
        using IsSimpleType_v = Error;
        int64_t number = 0;
        ::std::string message;
        static void WriteTo(xx::Data& d, int64_t const& number, std::string_view const& message);
    };
}
namespace Generic {
    struct PlayerInfo : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerInfo, ::xx::ObjBase)
        using IsSimpleType_v = PlayerInfo;
        int64_t id = 0;
        ::std::string username;
        ::std::string password;
        ::std::string nickname;
        int64_t gold = 0;
        static void WriteTo(xx::Data& d, int64_t const& id, std::string_view const& username, std::string_view const& password, std::string_view const& nickname, int64_t const& gold);
    };
}
#include <pkg_.h.inc>
