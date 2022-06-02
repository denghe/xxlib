#pragma once
#include <xx_obj.h>
#include <pkg.h.inc>
struct CodeGen_pkg {
	inline static const ::std::string md5 = "#*MD5<102333442c921ada0fa305fe93bfe09c>*#";
    static void Register();
    CodeGen_pkg() { Register(); }
};
inline CodeGen_pkg __CodeGen_pkg;
namespace Generic { struct PlayerInfo; }
namespace Generic { struct Success; }
namespace Generic { struct Register; }
namespace All_Db { struct SetPlayerGold; }
namespace All_Db { struct GetPlayerInfo; }
namespace All_Db { struct GetPlayerId; }
namespace Lobby_Game1 { struct PlayerEnter; }
namespace Game1_Lobby { struct PlayerLeave; }
namespace Client_Lobby { struct EnterGame; }
namespace Client_Lobby { struct Login; }
namespace Client_Game1 { struct PlayerLeave; }
namespace Client_Game1 { struct AddGold; }
namespace Client_Game1 { struct Enter; }
namespace Client_Game1 { struct Loading; }
namespace Game1_Client { struct Scene; }
namespace Lobby_Client { struct Scene; }
struct Pong;
namespace Generic { struct Error; }
struct Ping;
namespace xx {
    template<> struct TypeId<::Generic::PlayerInfo> { static const uint16_t value = 14; };
    template<> struct TypeId<::Generic::Success> { static const uint16_t value = 12; };
    template<> struct TypeId<::Generic::Register> { static const uint16_t value = 11; };
    template<> struct TypeId<::All_Db::SetPlayerGold> { static const uint16_t value = 203; };
    template<> struct TypeId<::All_Db::GetPlayerInfo> { static const uint16_t value = 202; };
    template<> struct TypeId<::All_Db::GetPlayerId> { static const uint16_t value = 201; };
    template<> struct TypeId<::Lobby_Game1::PlayerEnter> { static const uint16_t value = 301; };
    template<> struct TypeId<::Game1_Lobby::PlayerLeave> { static const uint16_t value = 401; };
    template<> struct TypeId<::Client_Lobby::EnterGame> { static const uint16_t value = 502; };
    template<> struct TypeId<::Client_Lobby::Login> { static const uint16_t value = 501; };
    template<> struct TypeId<::Client_Game1::PlayerLeave> { static const uint16_t value = 604; };
    template<> struct TypeId<::Client_Game1::AddGold> { static const uint16_t value = 603; };
    template<> struct TypeId<::Client_Game1::Enter> { static const uint16_t value = 602; };
    template<> struct TypeId<::Client_Game1::Loading> { static const uint16_t value = 601; };
    template<> struct TypeId<::Game1_Client::Scene> { static const uint16_t value = 701; };
    template<> struct TypeId<::Lobby_Client::Scene> { static const uint16_t value = 801; };
    template<> struct TypeId<::Pong> { static const uint16_t value = 1283; };
    template<> struct TypeId<::Generic::Error> { static const uint16_t value = 13; };
    template<> struct TypeId<::Ping> { static const uint16_t value = 2062; };
}

namespace Game1 {
    struct PlayerInfo {
        XX_OBJ_STRUCT_H(PlayerInfo)
        using IsSimpleType_v = PlayerInfo;
        int64_t playerId = 0;
        ::std::string nickname;
        int64_t gold = 0;
    };
}
namespace Generic {
    // response
    struct PlayerInfo : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerInfo, ::xx::ObjBase)
        using IsSimpleType_v = PlayerInfo;
#include <pkg_GenericPlayerInfo.inc>
        int64_t id = 0;
        ::std::string username;
        ::std::string password;
        ::std::string nickname;
        int64_t gold = 0;
        static void WriteTo(xx::Data& d, int64_t const& id, std::string_view const& username, std::string_view const& password, std::string_view const& nickname, int64_t const& gold);
    };
}
namespace Generic {
    // response
    struct Success : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Success, ::xx::ObjBase)
        using IsSimpleType_v = Success;
        int64_t value = 0;
        static void WriteTo(xx::Data& d, int64_t const& value);
    };
}
namespace Generic {
    // push
    struct Register : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Register, ::xx::ObjBase)
        using IsSimpleType_v = Register;
        uint32_t id = 0;
        static void WriteTo(xx::Data& d, uint32_t const& id);
    };
}
namespace All_Db {
    // request. return Generic.Error || Generic.Success{ value = gold }
    struct SetPlayerGold : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(SetPlayerGold, ::xx::ObjBase)
        using IsSimpleType_v = SetPlayerGold;
        int64_t id = 0;
        int64_t gold = 0;
        static void WriteTo(xx::Data& d, int64_t const& id, int64_t const& gold);
    };
}
namespace All_Db {
    // request. return Generic.Error || Generic.PlayerInfo
    struct GetPlayerInfo : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetPlayerInfo, ::xx::ObjBase)
        using IsSimpleType_v = GetPlayerInfo;
        int64_t id = 0;
        static void WriteTo(xx::Data& d, int64_t const& id);
    };
}
namespace All_Db {
    // request. return Generic.Error || Generic.Success{ value = id }
    struct GetPlayerId : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetPlayerId, ::xx::ObjBase)
        using IsSimpleType_v = GetPlayerId;
        ::std::string username;
        ::std::string password;
        static void WriteTo(xx::Data& d, std::string_view const& username, std::string_view const& password);
    };
}
namespace Lobby_Game1 {
    // request. return Generic.Error || Generic.Success{}
    struct PlayerEnter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerEnter, ::xx::ObjBase)
        using IsSimpleType_v = PlayerEnter;
        uint32_t gatewayId = 0;
        uint32_t clientId = 0;
        int64_t playerId = 0;
        static void WriteTo(xx::Data& d, uint32_t const& gatewayId, uint32_t const& clientId, int64_t const& playerId);
    };
}
namespace Game1_Lobby {
    // request. return Generic.Error || Generic.Success{}
    struct PlayerLeave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerLeave, ::xx::ObjBase)
        using IsSimpleType_v = PlayerLeave;
        int64_t playerId = 0;
        static void WriteTo(xx::Data& d, int64_t const& playerId);
    };
}
namespace Client_Lobby {
    // request. return Generic.Error || Generic.Success{ value = server id }
    struct EnterGame : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(EnterGame, ::xx::ObjBase)
        using IsSimpleType_v = EnterGame;
        // game id, level id, room id, sit id ...
        ::std::vector<int32_t> gamePath;
        static void WriteTo(xx::Data& d, ::std::vector<int32_t> const& gamePath);
    };
}
namespace Client_Lobby {
    // request. return Generic.Error || Generic.Success{ value = player id } + Push Scene
    struct Login : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Login, ::xx::ObjBase)
        using IsSimpleType_v = Login;
        ::std::string username;
        ::std::string password;
        static void WriteTo(xx::Data& d, std::string_view const& username, std::string_view const& password);
    };
}
namespace Client_Game1 {
    // request. return Generic.Error || Generic.Success{ value = server id }
    struct PlayerLeave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(PlayerLeave, ::xx::ObjBase)
        using IsSimpleType_v = PlayerLeave;
        static void WriteTo(xx::Data& d);
    };
}
namespace Client_Game1 {
    // push. game logic command
    struct AddGold : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(AddGold, ::xx::ObjBase)
        using IsSimpleType_v = AddGold;
        int64_t value = 0;
        static void WriteTo(xx::Data& d, int64_t const& value);
    };
}
namespace Client_Game1 {
    // push. loading finished. enter game. will receive push Scene
    struct Enter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Enter, ::xx::ObjBase)
        using IsSimpleType_v = Enter;
        static void WriteTo(xx::Data& d);
    };
}
namespace Client_Game1 {
    // push. refresh game server's player online time, avoid kick
    struct Loading : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Loading, ::xx::ObjBase)
        using IsSimpleType_v = Loading;
        static void WriteTo(xx::Data& d);
    };
}
namespace Game1_Client {
    // push
    struct Scene : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Scene, ::xx::ObjBase)
        using IsSimpleType_v = Scene;
        ::std::map<int32_t, ::Game1::PlayerInfo> players;
        static void WriteTo(xx::Data& d, ::std::map<int32_t, ::Game1::PlayerInfo> const& players);
    };
}
namespace Lobby_Client {
    // push
    struct Scene : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Scene, ::xx::ObjBase)
        ::xx::Shared<::Generic::PlayerInfo> playerInfo;
        ::std::string gamesIntro;
    };
}
struct Pong : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(Pong, ::xx::ObjBase)
    using IsSimpleType_v = Pong;
    int64_t ticks = 0;
    static void WriteTo(xx::Data& d, int64_t const& ticks);
};
namespace Generic {
    // response
    struct Error : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Error, ::xx::ObjBase)
        using IsSimpleType_v = Error;
        int64_t number = 0;
        ::std::string message;
        static void WriteTo(xx::Data& d, int64_t const& number, std::string_view const& message);
    };
}
// return Pong{ value = Ping.ticks }
struct Ping : ::xx::ObjBase {
    XX_OBJ_OBJECT_H(Ping, ::xx::ObjBase)
    using IsSimpleType_v = Ping;
    int64_t ticks = 0;
    static void WriteTo(xx::Data& d, int64_t const& ticks);
};
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::Game1::PlayerInfo)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::Game1::PlayerInfo, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)-1).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)-1).Read(d, out); }
    };
}
#include <pkg_.h.inc>
