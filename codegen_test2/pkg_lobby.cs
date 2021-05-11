using TemplateLibrary;

namespace Client_Lobby.Request {
    [TypeId(10)]
    class Auth {
        string username;
        string password;
    }
}

namespace Lobby_Client.Response {
    namespace Auth {
        [TypeId(11)]
        class Error : Generic.Error {};

        [TypeId(12)]
        class Online {
            int accountId;
            // todo: more account data here
        }

        [TypeId(13)]
        class Restore : Online {
            int serviceId;
            // todo: more state data here
        }
    }
}
