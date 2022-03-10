
var CodeGen_pkg_generic_md5 ='#*MD5<36740f71b858711bb264fc60bb537585>*#';

// 成功
class Generic_Success extends ObjBase {
    static typeId = 1;
    Read(d) {
    }
    Write(d) {
    }
}
Data.Register(Generic_Success);
// 出错
class Generic_Error extends ObjBase {
    static typeId = 2;
    errorCode = 0; // Int32
    errorMessage = ""; // String
    Read(d) {
        this.errorCode = d.Rvi();
        this.errorMessage = d.Rstr();
    }
    Write(d) {
        d.Wvi(this.errorCode);
        d.Wstr(this.errorMessage);
    }
}
Data.Register(Generic_Error);
// 游戏信息
class Generic_GameInfo {
    // 游戏标识
    gameId = 0; // Int32
    // 游戏说明( 服务器并不关心, 会转发到 client. 通常是一段 json 啥的 )
    info = ""; // String
    Read(d) {
        this.gameId = d.Rvi();
        this.info = d.Rstr();
    }
    Write(d) {
        d.Wvi(this.gameId);
        d.Wstr(this.info);
    }
}
// 玩家公开基础信息
class Generic_PlayerInfo {
    // 玩家标识
    accountId = 0; // Int32
    // 玩家昵称 / 显示名
    nickname = ""; // String
    // 玩家资产
    coin = 0; // Double
    Read(d) {
        this.accountId = d.Rvi();
        this.nickname = d.Rstr();
        this.coin = d.Rd();
    }
    Write(d) {
        d.Wvi(this.accountId);
        d.Wstr(this.nickname);
        d.Wd(this.coin);
    }
}