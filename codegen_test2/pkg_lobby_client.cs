using TemplateLibrary;

namespace Client_Lobby {
    [TypeId(10)]
    [Desc("请求：登录/校验. 成功返回 Lobby_Client.PlayerContext ( 也可能返回排队之类 ), 出错返回 Generic.Error")]
    class Auth {
        string username;
        string password;
        // more ...
    }

    [TypeId(11)]
    [Desc("请求：进入游戏. 成功返回 Lobby_Client.EnterGameSuccess, 出错返回 Generic.Error")]
    class EnterGame {
        int gameId;
    }
}

namespace Lobby_Client {
    [Desc("回应：登录成功( 新上线/顶下线 或 恢复游戏 ). 通常接下来将推送 GameOpen")]
    [TypeId(20)]
    class PlayerContext {
        [Desc("自己( 当前账号 )的基础信息")]
        Generic.PlayerInfo self;
        [Desc("当前所在游戏id( 小于0: 在大厅 )")]
        int gameId;
        [Desc("要等待 open 的服务id( 小于0: 新上线 不用等  等于0: 大厅顶下线 不用等  大于0: 游戏服务id )")]
        int serviceId;
    }

    [Desc("回应：进游戏成功")]
    [TypeId(21)]
    class EnterGameSuccess {
        [Desc("要等待 open 的服务id")]
        int serviceId;
    }

    [Desc("推送: 游戏开服")]
    [TypeId(30)]
    class GameOpen {
        [Desc("游戏信息列表")]
        List<Generic.GameInfo> gameInfos;
    }

    [Desc("推送: 游戏关服")]
    [TypeId(31)]
    class GameClose {
        [Desc("游戏标识列表")]
        List<int> gameIds;
    }
}
