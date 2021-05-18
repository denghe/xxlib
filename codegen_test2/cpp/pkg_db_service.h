#pragma once
#include "pkg_generic.h"
#include "pkg_db_service.h.inc"
struct CodeGen_pkg_db_service {
	inline static const ::std::string md5 = "#*MD5<2471f2de3593c978d18c657ef65bf8cc>*#";
    static void Register();
    CodeGen_pkg_db_service() { Register(); }
};
inline CodeGen_pkg_db_service __CodeGen_pkg_db_service;
namespace Database_Service { struct GetAccountInfoByUsernamePasswordResult; }
namespace Service_Database { struct GetAccountInfoByUsernamePassword; }
namespace xx {
    template<> struct TypeId<::Database_Service::GetAccountInfoByUsernamePasswordResult> { static const uint16_t value = 110; };
    template<> struct TypeId<::Service_Database::GetAccountInfoByUsernamePassword> { static const uint16_t value = 100; };
}

namespace Database {
    // 账号信息
    struct AccountInfo {
        XX_OBJ_STRUCT_H(AccountInfo)
        using IsSimpleType_v = AccountInfo;
        int32_t accountId = -1;
        ::std::string nickname;
        double coin = 0;
        static void WriteTo(xx::Data& d, int32_t const&, std::string_view const&, double const&);
    };
}
namespace Database_Service {
    // 回应：查询结果
    struct GetAccountInfoByUsernamePasswordResult : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetAccountInfoByUsernamePasswordResult, ::xx::ObjBase)
        using IsSimpleType_v = GetAccountInfoByUsernamePasswordResult;
        // 为空就是没找到
        ::std::optional<::Database::AccountInfo> accountInfo;
        static void WriteTo(xx::Data& d, ::std::optional<::Database::AccountInfo> const&);
    };
}
namespace Service_Database {
    // 请求：根据用户名密码拿账号信息。出错返回 Generic.Error. 成功返回 Database_Service.GetAccountInfoByUsernamePasswordResult
    struct GetAccountInfoByUsernamePassword : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(GetAccountInfoByUsernamePassword, ::xx::ObjBase)
        using IsSimpleType_v = GetAccountInfoByUsernamePassword;
        ::std::string username;
        ::std::string password;
        static void WriteTo(xx::Data& d, std::string_view const&, std::string_view const&);
    };
}
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::Database::AccountInfo)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::Database::AccountInfo, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)nullptr).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)nullptr).Read(d, out); }
    };
}
#include "pkg_db_service_.h.inc"
