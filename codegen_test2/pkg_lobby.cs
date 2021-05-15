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

        [Desc("登录成功 新上线 或 恢复")]
        [TypeId(12)]
        class Success {
            int accountId;
            string nickname;
            double coin;
            // more ...
            List<Game> games;
            [Desc("要回到的游戏id( 小于0: 在大厅 )")]
            int gameId;
            [Desc("要等待 open 的服务id( 小于0: 新上线  等于0: 大厅顶下线  大于0: 游戏服务id )")]
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
