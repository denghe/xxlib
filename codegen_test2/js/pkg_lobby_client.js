// <script language="javascript" src="pkg_generic.js"></script>

var CodeGen_pkg_lobby_client_md5 ='#*MD5<2748302cc6925099d2a18f2a2d7caf1a>*#';

// 回应：登录成功( 新上线/顶下线 或 恢复游戏 ). 通常接下来将推送 GameOpen
class Lobby_Client_PlayerContext extends ObjBase {
    static typeId = 20;
    // 自己( 当前账号 )的基础信息
    self = new Generic_PlayerInfo(); // Generic.PlayerInfo
    // 当前所在游戏id( 小于0: 在大厅 )
    gameId = 0; // Int32
    // 要等待 open 的服务id( 小于0: 新上线 不用等  等于0: 大厅顶下线 不用等  大于0: 游戏服务id )
    serviceId = 0; // Int32
    Read(d) {
        this.self.Read(d);
        this.gameId = d.Rvi();
        this.serviceId = d.Rvi();
    }
    Write(d) {
        this.self.Write(d);
        d.Wvi(this.gameId);
        d.Wvi(this.serviceId);
    }
}
Data.Register(Lobby_Client_PlayerContext);
// 回应：进游戏成功
class Lobby_Client_EnterGameSuccess extends ObjBase {
    static typeId = 21;
    // 要等待 open 的服务id
    serviceId = 0; // Int32
    Read(d) {
        this.serviceId = d.Rvi();
    }
    Write(d) {
        d.Wvi(this.serviceId);
    }
}
Data.Register(Lobby_Client_EnterGameSuccess);
// 推送: 游戏开服
class Lobby_Client_GameOpen extends ObjBase {
    static typeId = 30;
    // 游戏信息列表
    gameInfos = []; // List<Generic.GameInfo>
    Read(d) {
        let o, len;
        len = d.Rvu();
        o = [];
        this.gameInfos = o;
        for (let i = 0; i < len; ++i) {
            o[i] = new Generic_GameInfo();
            o[i].Read(d);
        }
    }
    Write(d) {
        let o, len;
        o = this.gameInfos;
        len = o.length;
        d.Wvu(len);
        for (let i = 0; i < len; ++i) {
            o[i].Write(d);
        }
    }
}
Data.Register(Lobby_Client_GameOpen);
// 推送: 游戏关服
class Lobby_Client_GameClose extends ObjBase {
    static typeId = 31;
    // 游戏标识列表
    gameIds = []; // List<Int32>
    Read(d) {
        let o, len;
        len = d.Rvu();
        o = [];
        this.gameIds = o;
        for (let i = 0; i < len; ++i) {
            o[i] = d.Rvi();
        }
    }
    Write(d) {
        let o, len;
        o = this.gameIds;
        len = o.length;
        d.Wvu(len);
        for (let i = 0; i < len; ++i) {
            d.Wvi(o[i]);
        }
    }
}
Data.Register(Lobby_Client_GameClose);
// 请求：登录/校验. 成功返回 Lobby_Client.PlayerContext ( 也可能返回排队之类 ), 出错返回 Generic.Error
class Client_Lobby_Auth extends ObjBase {
    static typeId = 10;
    username = ""; // String
    password = ""; // String
    Read(d) {
        this.username = d.Rstr();
        this.password = d.Rstr();
    }
    Write(d) {
        d.Wstr(this.username);
        d.Wstr(this.password);
    }
}
Data.Register(Client_Lobby_Auth);
// 请求：进入游戏. 成功返回 Lobby_Client.EnterGameSuccess, 出错返回 Generic.Error
class Client_Lobby_EnterGame extends ObjBase {
    static typeId = 11;
    gameId = 0; // Int32
    Read(d) {
        this.gameId = d.Rvi();
    }
    Write(d) {
        d.Wvi(this.gameId);
    }
}
Data.Register(Client_Lobby_EnterGame);