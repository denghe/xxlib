using TemplateLibrary;

[TypeId(2062), Desc("return Pong{ value = Ping.ticks }")]
class Ping {
    long ticks;
}

[TypeId(1283)]
class Pong {
    long ticks;
}

namespace Generic {
    [TypeId(11), Desc("push")]
    class Register {
        uint id;
    }

    [TypeId(12), Desc("response")]
    class Success {
        long value;
    }

    [TypeId(13), Desc("response")]
    class Error {
        long number;
        string message;
    }

    [TypeId(14), Desc("response"), Include]
    class PlayerInfo {
        long id;
        string username;
        string password;

        string nickname;
        long gold;
    }
}

namespace All_Db {
    [TypeId(201), Desc("request. return Generic.Error || Generic.Success{ value = id }")]
    class GetPlayerId {
        string username;
        string password;
    }

    [TypeId(202), Desc("request. return Generic.Error || Generic.PlayerInfo")]
    class GetPlayerInfo {
        long id;
    }
}

namespace Lobby_Game1 {
    [TypeId(301), Desc("request. return Generic.Error || Generic.Success{}")]
    class PlayerEnter {
        uint gatewayId;
        uint clientId;
        long playerId;
    }
}

namespace Game1_Lobby {
    [TypeId(401), Desc("request. return Generic.Error || Generic.Success{}")]
    class PlayerLeave {
        long playerId;
    }
}

namespace Client_Lobby {
    [TypeId(501), Desc("request. return Generic.Error || Generic.Success{ value = player id } + Push Scene")]
    class Login {
        string username;
        string password;
    }

    [TypeId(502), Desc("request. return Generic.Error || Generic.Success{ value = server id }")]
    class EnterGame {
        [Desc("game id, level id, room id, sit id ...")]
        List<int> gamePath;
    }
}

namespace Game1 {
    struct PlayerInfo {
        long playerId;
        string nickname;
        long gold;
    }
}

namespace Client_Game1 {
    [TypeId(601), Desc("push. refresh game server's player online time, avoid kick")]
    class Loading {
    }

    [TypeId(602), Desc("push. loading finished. enter game. will receive push Scene")]
    class Enter {
    }

    [TypeId(603), Desc("push. game logic command")]
    class AddGold {
        long value;
    }

    [TypeId(604), Desc("request. return Generic.Error || Generic.Success{ value = server id }")]
    class PlayerLeave {
    }
}

namespace Game1_Client {
    [TypeId(701), Desc("push")]
    class Scene {
        Dict<int, Game1.PlayerInfo> players;
    }
}

namespace Lobby_Client {
    [TypeId(801), Desc("push")]
    class Scene {
        Shared<Generic.PlayerInfo> playerInfo;
        string gamesIntro;
    }
}
