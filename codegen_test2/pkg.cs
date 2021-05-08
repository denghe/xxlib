using TemplateLibrary;

namespace Client_Lobby {
    [TypeId(10)]
    class Auth {
        string username;
        string password;
    };
}

namespace Lobby_Client {
    [TypeId(11)]
    class AuthResult {
        [Desc("-1: not found")]
        int accountId;
    };
}
