using TemplateLibrary;

[TypeId(2062), Desc("return Pong{ value = Ping.ticks }")]
class Ping {
    long ticks;
}

[TypeId(1283)]
class Pong {
    long ticks;
}

namespace Generic
{
    [TypeId(10)]
    class Success
    {
        long value;
    }

    [TypeId(11)]
    class Error
    {
        long number;
        string message;
    }

    [TypeId(12)]
    class PlayerInfo
    {
        long id;
        string username;
        string password;

        string nickname;
        long gold;
    }
}

namespace All_Db
{
    [TypeId(201), Desc("return Generic.Error || Generic.Success{ value = id }")]
    class GetPlayerId
    {
        string username;
        string password;
    }

    [TypeId(202), Desc("return Generic.Error || Generic.PlayerInfo")]
    class GetPlayerInfo
    {
        long id;
    }
}

namespace Lobby_Game1
{
    [TypeId(301), Desc("return Generic.Error || Generic.Success{}")]
    class PlayerEnter
    {
        uint gatewayId;
        uint clientId;
        long playerId;
    }
}

namespace Game1_Lobby
{
    [TypeId(401), Desc("return Generic.Error || Generic.Success{}")]
    class PlayerLeave
    {
        long playerId;
    }
}

namespace Client_Lobby
{
    [TypeId(501), Desc("return Generic.Error || Generic.Success{ value = id } + Push Scene")]
    class Login
    {
        string username;
        string password;
    }
}

namespace Game1
{
    struct PlayerInfo
    {
        long playerId;
        string nickname;
        long gold;
    }
}

namespace Client_Game1
{
    [TypeId(601), Desc("push")]
    class AddGold
    {
        long value;
    }

    [TypeId(602), Desc("return Generic.Error || Generic.Success{}")]
    class PlayerLeave
    {
    }
}

namespace Game1_Client
{
    [TypeId(701), Desc("push")]
    class Scene
    {
        Dict<int, Game1.PlayerInfo> players;
    }
}

namespace Lobby_Client
{
    [TypeId(801), Desc("push")]
    class Scene
    {
        Shared<Generic.PlayerInfo> playerInfo;
        string gamesIntro;
    }
}
