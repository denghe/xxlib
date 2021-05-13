#pragma once
#include "pkg_generic.h"
#include "pkg_db.h.inc"
struct CodeGen_pkg_db {
	inline static const ::std::string md5 = "#*MD5<79d27406f490a9cfd9b5fca92c716427>*#";
    static void Register();
    CodeGen_pkg_db() { Register(); }
};
inline CodeGen_pkg_db __CodeGen_pkg_db;
namespace Database_Lobby::GetAccountInfoByUsernamePassword { struct Error; }
namespace Database_Lobby::GetAccountInfoByUsernamePassword { struct Result; }
namespace Lobby_Database { struct GetAccountInfoByUsernamePassword; }
namespace xx {
    template<> struct TypeId<::Database_Lobby::GetAccountInfoByUsernamePassword::Error> { static const uint16_t value = 21; };
    template<> struct TypeId<::Database_Lobby::GetAccountInfoByUsernamePassword::Result> { static const uint16_t value = 22; };
    template<> struct TypeId<::Lobby_Database::GetAccountInfoByUsernamePassword> { static const uint16_t value = 20; };
}

namespace Database {
    // 账号信息
    struct AccountInfo {
        XX_OBJ_STRUCT_H(AccountInfo)
        using IsSimpleType_v = AccountInfo;
        int32_t accountId = -1;
        ::std::string nickname;
        double coin = 0;
    };
}
namespace Database_Lobby::GetAccountInfoByUsernamePassword {
    // 查询出错
    struct Error : ::Generic::Error {
        XX_OBJ_OBJECT_H(Error, ::Generic::Error)
        using IsSimpleType_v = Error;
    };
}
namespace Database_Lobby::GetAccountInfoByUsernamePassword {
    // 查询结果
    struct Result : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Result, ::xx::ObjBase)
        using IsSimpleType_v = Result;
        // 为空就是没找到
        ::std::optional<::Database::AccountInfo> accountInfo;
    };
}
namespace Lobby_Database {
    // 根据用户名密码拿账号信息
    struct GetAccountInfoByUsernamePassword : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetAccountInfoByUsernamePassword, ::xx::ObjBase)
        using IsSimpleType_v = GetAccountInfoByUsernamePassword;
        ::std::string username;
        ::std::string password;
    };
}
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::Database::AccountInfo)
}
#include "pkg_db_.h.inc"
