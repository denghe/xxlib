using TemplateLibrary;

namespace Database {
    [Struct, Desc("账号信息")]
    class AccountInfo {
        int accountId = -1;
        string nickname;
        double coin = 0;
    }
}

namespace Lobby_Database {
    [Desc("根据用户名密码拿账号信息")]
    [TypeId(20)]
    class GetAccountInfoByUsernamePassword {
        string username;
        string password;
    }
}

namespace Database_Lobby {
    namespace GetAccountInfoByUsernamePassword {
        [Desc("查询出错")]
        [TypeId(21)]
        class Error : Generic.Error {};

        [Desc("查询结果")]
        [TypeId(22)]
        class Result {
            [Desc("为空就是没找到")]
            Nullable<Database.AccountInfo> accountInfo;
        }
    }
}
