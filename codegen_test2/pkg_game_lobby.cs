using TemplateLibrary;

namespace Game_Lobby {
    [TypeId(200)]
    [Desc("请求：注册游戏. 一个服务可注册多个游戏. 注册后, 已在线客户端将收到开服通知，成功返回 Generic.Success. 错误或异常 返回 Generic.Error")]
    class Register {
        uint serviceId;
        List<Generic.GameInfo> gameInfos;
    }

    [TypeId(201)]
    [Desc("推送：玩家已退出游戏")]
    class PlayerLeave {
        [Desc("玩家标识")]
        int accountId;
        int gameId;
    }
}

namespace Lobby_Game {
    [TypeId(210)]
    [Desc("请求：玩家进入. 成功返回 Generic.Success. 错误或异常 返回 Generic.Error")]
    class PlayerEnter {
        [Desc("网关标识")]
        uint gatewayId;
        [Desc("网关连接标识")]
        uint clientId;
        [Desc("进入哪个游戏")]
        int gameId;
        [Desc("玩家信息顺便传递，避免查询 db")]
        Generic.PlayerInfo playerInfo;
    }

    [TypeId(211)]
    [Desc("请求：强制玩家立刻退出游戏( 断线, 结存, 清除上下文 ). 操作完毕后返回 Generic.Success. 错误或异常 返回 Generic.Error")]
    class PlayerLeave {
        [Desc("玩家标识")]
        int accountId;
        [Desc("操作原因")]
        string reason;
    }
}
