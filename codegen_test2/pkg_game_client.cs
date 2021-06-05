using TemplateLibrary;

namespace Game1 {
    [Desc("玩家( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )")]
    [TypeId(50), Include_]
    class Player {
        Generic.PlayerInfo info;
    }

    [Desc("玩家消息( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )")]
    [TypeId(51)]
    class Message {
        [Desc("序号 / 时间戳( 游戏服取当前 tick 生成并保存最后一次的值. 如果 当前 tick 小于最后一次值，就使用 ++最后一次值 )")]
        long timestamp;
        [Desc("发送者 accountId")]
        int senderId;
        [Desc("发送者 nickname ( 缓存一份避免 player 离开后查不到 )")]
        string senderNickname;
        [Desc("内容")]
        string content;
    }

    [Desc("游戏场景( 不便下发到 client 的敏感信息不在此列，由服务端自行附加 )")]
    [TypeId(52), Include_]
    class Scene {
        Generic.GameInfo gameInfo;
        List<Shared<Player>> players;
        List<Shared<Message>> messages;
    }

    [Desc("事件--基类")]
    [TypeId(70)]
    class Event {
        [Desc("同 Game1.Message.timestamp")]
        long timestamp;
    }

    [Desc("事件--玩家进入")]
    [TypeId(71)]
    class Event_PlayerEnter : Event {
        [Desc("玩家信息")]
        Shared<Player> player;
    }

    [Desc("事件--玩家离开")]
    [TypeId(72)]
    class Event_PlayerLeave : Event {
        int accountId;
    }

    [Desc("事件--玩家发消息")]
    [TypeId(73)]
    class Event_Message : Event {
        [Desc("发送者 accountId")]
        int senderId;
        [Desc("内容")]
        string content;
    }
}

namespace Client_Game1 {
    [Desc("推送: 进入游戏")]
    [TypeId(60)]
    class Enter {
        [Desc("是否正在读条. 是：并非真正进入，只是为了延长 游戏服超时踢人 时间, 无返回值. 否: 游戏服下发 FullSync")]
        bool loading;
    }

    [Desc("请求：离开游戏. 游戏服 结算(可能和db服有交互)，通知大厅某玩家离开, 最后返回：Generic.Success, 出错返回 Generic.Error")]
    [TypeId(61)]
    class Leave {
    }

    [Desc("推送: 发消息")]
    [TypeId(62)]
    class SendMessage {
        [Desc("内容")]
        string content;
    }
}

namespace Game1_Client {
    [Desc("推送: 完整同步")]
    [TypeId(80)]
    class FullSync {
        [Desc("游戏场景")]
        Shared<Game1.Scene> scene;
        [Desc("指向当前玩家上下文")]
        Weak<Game1.Player> self;
    }

    [Desc("推送: 事件同步")]
    [TypeId(81)]
    class Sync {
        [Desc("事件集合( 已按时间戳排序 )")]
        List<Shared<Game1.Event>> events;
    }
}
