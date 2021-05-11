using TemplateLibrary;

namespace Client_Lobby {
    [TypeId(10)]
    [Desc("登录/校验")]
    class Auth {
        string username;
        string password;
    }
}

namespace Lobby_Client {

    [Desc("游戏信息")]
    struct Game {
        int gameId;

        // more ...
    }

    // 登录/校验 的返回值
    namespace Auth {
        [Desc("登录出错")]
        [TypeId(11)]
        class Error : Generic.Error {};

        [Desc("登录成功 上线( 不在游戏中的时候，即便是断线重连也下发这个包 )")]
        [TypeId(12)]
        class Online {
            int accountId;
            string nickname;
            double coin;
            // more ...
            List<Game> games;
        }

        [Desc("登录成功 恢复( 断线重连, 回到游戏 )")]
        [TypeId(13)]
        class Restore : Online {
            [Desc("要回到的游戏id( 界面切换、预加载的凭据 )")]
            int gameId;
            [Desc("要等待 open 的服务id( 等待网络就绪，和游戏服通信的地址 )")]
            int serviceId;
        }
    }
}


/*

struct Game : Place {
    int gameId;
    map<int, Weak<Player>> players;
    Shared<SPeer> peer;
}

struct SPeer {
    Weak<Game> game;
}

struct Server {
    map<int, Shared<Game>> games;
}

class Player {
    int accountId;      // fill from db
    string nickname;    // fill from db
    double coin;        // fill from db

    Weak<Game> game;

    ~Player() {
      if auto g = game.Lock() {
        g->players.erase( accountId )
      }
    }
}

*/
