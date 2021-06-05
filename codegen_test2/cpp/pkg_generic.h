#pragma once
#include <xx_obj.h>
#include <pkg_generic.h.inc>
struct CodeGen_pkg_generic {
	inline static const ::std::string md5 = "#*MD5<d26fb32354a5f194e227dd18ba274c57>*#";
    static void Register();
    CodeGen_pkg_generic() { Register(); }
};
inline CodeGen_pkg_generic __CodeGen_pkg_generic;
namespace Generic { struct Success; }
namespace Generic { struct Error; }
namespace xx {
    template<> struct TypeId<::Generic::Success> { static const uint16_t value = 1; };
    template<> struct TypeId<::Generic::Error> { static const uint16_t value = 2; };
}

namespace Generic {
    // 游戏信息
    struct GameInfo {
        XX_OBJ_STRUCT_H(GameInfo)
        using IsSimpleType_v = GameInfo;
        // 游戏标识
        int32_t gameId = 0;
        // 游戏说明( 服务器并不关心, 会转发到 client. 通常是一段 json 啥的 )
        ::std::string info;
    };
}
namespace Generic {
    // 玩家公开基础信息
    struct PlayerInfo {
        XX_OBJ_STRUCT_H(PlayerInfo)
        using IsSimpleType_v = PlayerInfo;
        // 玩家标识
        int32_t accountId = 0;
        // 玩家昵称 / 显示名
        ::std::string nickname;
        // 玩家资产
        double coin = 0;
    };
}
namespace Generic {
    // 成功
    struct Success : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Success, ::xx::ObjBase)
        using IsSimpleType_v = Success;
        static void WriteTo(xx::Data& d);
    };
}
namespace Generic {
    // 出错
    struct Error : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Error, ::xx::ObjBase)
        using IsSimpleType_v = Error;
        int32_t errorCode = 0;
        ::std::string errorMessage;
        static void WriteTo(xx::Data& d, int32_t const& errorCode, std::string_view const& errorMessage);
    };
}
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::Generic::GameInfo)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::Generic::GameInfo, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)-1).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)-1).Read(d, out); }
    };
	XX_OBJ_STRUCT_TEMPLATE_H(::Generic::PlayerInfo)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::Generic::PlayerInfo, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)-1).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)-1).Read(d, out); }
    };
}
#include <pkg_generic_.h.inc>
