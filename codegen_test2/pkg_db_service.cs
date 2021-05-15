using TemplateLibrary;

namespace Database {
    [Struct, Desc("账号信息")]
    class AccountInfo {
        int accountId = -1;
        string nickname;
        double coin = 0;
        // ...
    }
}

namespace Service_Database {
    [Desc("请求：根据用户名密码拿账号信息。出错返回 Generic.Error. 成功返回 Database_Service.GetAccountInfoByUsernamePasswordResult")]
    [TypeId(100)]
    class GetAccountInfoByUsernamePassword {
        string username;
        string password;
    }
}

namespace Database_Service {
    [Desc("回应：查询结果")]
    [TypeId(110)]
    class GetAccountInfoByUsernamePasswordResult {
        [Desc("为空就是没找到")]
        Nullable<Database.AccountInfo> accountInfo;
    }
}
