using TemplateLibrary;

[TypeId(2062)]
class Ping {
    long ticks;
};

[TypeId(1283)]
class Pong {
    long ticks;
};

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
    };

    [TypeId(202), Desc("return Generic.Error || Generic.PlayerInfo")]
    class GetPlayerInfo
    {
        long id;
    };
}

namespace Lobby_Game1
{
    [TypeId(301), Desc("return Generic.Error || Generic.Success{}")]
    class PlayerEnter
    {
        uint gatewayId;
        uint clientId;
        long playerId;
    };
}

namespace Game1_Lobby
{
    [TypeId(401), Desc("return Generic.Error || Generic.Success{}")]
    class PlayerLeave
    {
        long playerId;
    };
}
