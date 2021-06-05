#pragma once
#include <pkg_generic.h>
#include <pkg_game_client.h.inc>
struct CodeGen_pkg_game_client {
	inline static const ::std::string md5 = "#*MD5<41d9d759ff89ddeab2e80dbb7ecb71ee>*#";
    static void Register();
    CodeGen_pkg_game_client() { Register(); }
};
inline CodeGen_pkg_game_client __CodeGen_pkg_game_client;
namespace Game1 { struct Event; }
namespace Game1 { struct Player; }
namespace Game1 { struct Message; }
namespace Game1 { struct Scene; }
namespace Game1_Client { struct FullSync; }
namespace Game1_Client { struct Sync; }
namespace Client_Game1 { struct Enter; }
namespace Client_Game1 { struct Leave; }
namespace Client_Game1 { struct SendMessage; }
namespace Game1 { struct Event_PlayerEnter; }
namespace Game1 { struct Event_PlayerLeave; }
namespace Game1 { struct Event_Message; }
namespace xx {
    template<> struct TypeId<::Game1::Event> { static const uint16_t value = 70; };
    template<> struct TypeId<::Game1::Player> { static const uint16_t value = 50; };
    template<> struct TypeId<::Game1::Message> { static const uint16_t value = 51; };
    template<> struct TypeId<::Game1::Scene> { static const uint16_t value = 52; };
    template<> struct TypeId<::Game1_Client::FullSync> { static const uint16_t value = 80; };
    template<> struct TypeId<::Game1_Client::Sync> { static const uint16_t value = 81; };
    template<> struct TypeId<::Client_Game1::Enter> { static const uint16_t value = 60; };
    template<> struct TypeId<::Client_Game1::Leave> { static const uint16_t value = 61; };
    template<> struct TypeId<::Client_Game1::SendMessage> { static const uint16_t value = 62; };
    template<> struct TypeId<::Game1::Event_PlayerEnter> { static const uint16_t value = 71; };
    template<> struct TypeId<::Game1::Event_PlayerLeave> { static const uint16_t value = 72; };
    template<> struct TypeId<::Game1::Event_Message> { static const uint16_t value = 73; };
}

namespace Game1 {
    // 事件--基类
    struct Event : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Event, ::xx::ObjBase)
        // 同 Game1.Message.timestamp
        int64_t timestamp = 0;
        static void WriteTo(xx::Data& d, int64_t const& timestamp);
    };
}
namespace Game1 {
    // 玩家( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )
    struct Player : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Player, ::xx::ObjBase)
        using IsSimpleType_v = Player;
        ::Generic::PlayerInfo info;
        static void WriteTo(xx::Data& d, ::Generic::PlayerInfo const& info);
#include <pkg_game_client_Game1Player_.inc>
    };
}
namespace Game1 {
    // 玩家消息( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )
    struct Message : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Message, ::xx::ObjBase)
        using IsSimpleType_v = Message;
        // 序号 / 时间戳( 游戏服取当前 tick 生成并保存最后一次的值. 如果 当前 tick 小于最后一次值，就使用 ++最后一次值 )
        int64_t timestamp = 0;
        // 发送者 accountId
        int32_t senderId = 0;
        // 发送者 nickname ( 缓存一份避免 player 离开后查不到 )
        ::std::string senderNickname;
        // 内容
        ::std::string content;
        static void WriteTo(xx::Data& d, int64_t const& timestamp, int32_t const& senderId, std::string_view const& senderNickname, std::string_view const& content);
    };
}
namespace Game1 {
    // 游戏场景( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )
    struct Scene : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Scene, ::xx::ObjBase)
        using IsSimpleType_v = Scene;
        ::Generic::GameInfo gameInfo;
        ::std::vector<::xx::Shared<::Game1::Player>> players;
        ::std::vector<::xx::Shared<::Game1::Message>> messages;
        static void WriteTo(xx::Data& d, ::Generic::GameInfo const& gameInfo, ::std::vector<::xx::Shared<::Game1::Player>> const& players, ::std::vector<::xx::Shared<::Game1::Message>> const& messages);
#include <pkg_game_client_Game1Scene_.inc>
    };
}
namespace Game1_Client {
    // 推送: 完整同步
    struct FullSync : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(FullSync, ::xx::ObjBase)
        using IsSimpleType_v = FullSync;
        // 游戏场景
        ::xx::Shared<::Game1::Scene> scene;
        // 指向当前玩家上下文
        ::xx::Weak<::Game1::Player> self;
        static void WriteTo(xx::Data& d, ::xx::Shared<::Game1::Scene> const& scene, ::xx::Weak<::Game1::Player> const& self);
    };
}
namespace Game1_Client {
    // 推送: 事件同步
    struct Sync : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Sync, ::xx::ObjBase)
        // 事件集合( 已按时间戳排序 )
        ::std::vector<::xx::Shared<::Game1::Event>> events;
        static void WriteTo(xx::Data& d, ::std::vector<::xx::Shared<::Game1::Event>> const& events);
    };
}
namespace Client_Game1 {
    // 推送: 进入游戏
    struct Enter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Enter, ::xx::ObjBase)
        using IsSimpleType_v = Enter;
        // 是否正在读条. 是：并非真正进入，只是为了延长 游戏服超时踢人 时间, 无返回值. 否: 游戏服下发 FullSync
        bool loading = false;
        static void WriteTo(xx::Data& d, bool const& loading);
    };
}
namespace Client_Game1 {
    // 请求：离开游戏. 游戏服 结算(可能和db服有交互)，通知大厅某玩家离开, 最后返回：Generic.Success, 出错返回 Generic.Error
    struct Leave : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Leave, ::xx::ObjBase)
        using IsSimpleType_v = Leave;
        static void WriteTo(xx::Data& d);
    };
}
namespace Client_Game1 {
    // 推送: 发消息
    struct SendMessage : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(SendMessage, ::xx::ObjBase)
        using IsSimpleType_v = SendMessage;
        // 内容
        ::std::string content;
        static void WriteTo(xx::Data& d, std::string_view const& content);
    };
}
namespace Game1 {
    // 事件--玩家进入
    struct Event_PlayerEnter : ::Game1::Event {
        XX_OBJ_OBJECT_H(Event_PlayerEnter, ::Game1::Event)
        using IsSimpleType_v = Event_PlayerEnter;
        // 玩家信息
        ::xx::Shared<::Game1::Player> player;
        static void WriteTo(xx::Data& d, int64_t const& timestamp, ::xx::Shared<::Game1::Player> const& player);
    };
}
namespace Game1 {
    // 事件--玩家离开
    struct Event_PlayerLeave : ::Game1::Event {
        XX_OBJ_OBJECT_H(Event_PlayerLeave, ::Game1::Event)
        using IsSimpleType_v = Event_PlayerLeave;
        int32_t accountId = 0;
        static void WriteTo(xx::Data& d, int64_t const& timestamp, int32_t const& accountId);
    };
}
namespace Game1 {
    // 事件--玩家发消息
    struct Event_Message : ::Game1::Event {
        XX_OBJ_OBJECT_H(Event_Message, ::Game1::Event)
        using IsSimpleType_v = Event_Message;
        // 发送者 accountId
        int32_t senderId = 0;
        // 内容
        ::std::string content;
        static void WriteTo(xx::Data& d, int64_t const& timestamp, int32_t const& senderId, std::string_view const& content);
    };
}
#include <pkg_game_client_.h.inc>
