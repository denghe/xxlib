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
    [TypeId(11), Desc("push")]
    class Register
    {
        uint id;
    }

    [TypeId(12)]
    class Success
    {
        long value;
    }

    [TypeId(13)]
    class Error
    {
        long number;
        string message;
    }

    [TypeId(14)]
    class PlayerInfo
    {
        long id;
        string username;
        string password;

        string nickname;
        long gold;
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

namespace Lobby_Client
{
    [TypeId(801), Desc("push")]
    class Scene
    {
        Shared<Generic.PlayerInfo> playerInfo;
        string gamesIntro;
    }
}
