using TemplateLibrary;

namespace Generic {
    [Desc("成功")]
    [TypeId(1)]
    class Success {};

    [Desc("出错")]
    [TypeId(2)]
    class Error {
        int errorCode;
        string errorMessage;
    };

    [Desc("游戏信息")]
    struct GameInfo {
        [Desc("游戏标识")]
        int gameId;
        [Desc("游戏说明( 服务器并不关心, 会转发到 client. 通常是一段 json 啥的 )")]
        string info;
        // more ...
    }

    [Desc("玩家公开基础信息")]
    struct PlayerInfo {
        [Desc("玩家标识")]
        int accountId;
        [Desc("玩家昵称 / 显示名")]
        string nickname;
        [Desc("玩家资产")]
        double coin;
        // more ...
    }
}
