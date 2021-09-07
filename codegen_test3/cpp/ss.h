#pragma once
#include <xx_obj.h>
#include <ss.h.inc>
struct CodeGen_ss {
	inline static const ::std::string md5 = "#*MD5<029fd1c90d3eb1ecde776b885e608cb3>*#";
    static void Register();
    CodeGen_ss() { Register(); }
};
inline CodeGen_ss __CodeGen_ss;
namespace SS { struct Bullet; }
namespace SS { struct Shooter; }
namespace SS { struct Scene; }
namespace SS_S2C { struct Sync; }
namespace SS_S2C { struct Event; }
namespace SS_C2S { struct Cmd; }
namespace xx {
    template<> struct TypeId<::SS::Bullet> { static const uint16_t value = 3; };
    template<> struct TypeId<::SS::Shooter> { static const uint16_t value = 2; };
    template<> struct TypeId<::SS::Scene> { static const uint16_t value = 1; };
    template<> struct TypeId<::SS_S2C::Sync> { static const uint16_t value = 20; };
    template<> struct TypeId<::SS_S2C::Event> { static const uint16_t value = 21; };
    template<> struct TypeId<::SS_C2S::Cmd> { static const uint16_t value = 10; };
}

namespace SS {
    struct XY {
        XX_OBJ_STRUCT_H(XY)
        using IsSimpleType_v = XY;
        int32_t x = 0;
        int32_t y = 0;
    };
}
namespace SS {
    struct ControlState {
        XX_OBJ_STRUCT_H(ControlState)
        using IsSimpleType_v = ControlState;
        ::SS::XY aimPos;
        bool moveLeft = false;
        bool moveRight = false;
        bool moveUp = false;
        bool moveDown = false;
        bool button1 = false;
        bool button2 = false;
    };
}
namespace SS {
    struct Bullet : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Bullet, ::xx::ObjBase)
        using IsSimpleType_v = Bullet;
#include <ss_SSBullet.inc>
        int32_t life = 0;
        ::SS::XY pos;
        ::SS::XY inc;
        static void WriteTo(xx::Data& d, int32_t const& life, ::SS::XY const& pos, ::SS::XY const& inc);
#include <ss_SSBullet_.inc>
    };
}
namespace SS {
    struct Shooter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Shooter, ::xx::ObjBase)
#include <ss_SSShooter.inc>
        float bodyAngle = 0.0f;
        int32_t moveDistancePerFrame = 10;
        ::SS::XY pos;
        ::std::vector<::xx::Shared<::SS::Bullet>> bullets;
        ::SS::ControlState cs;
#include <ss_SSShooter_.inc>
    };
}
namespace SS {
    struct Scene : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Scene, ::xx::ObjBase)
#include <ss_SSScene.inc>
        int32_t frameNumber = 0;
        ::xx::Shared<::SS::Shooter> shooter;
#include <ss_SSScene_.inc>
    };
}
namespace SS_S2C {
    struct Sync : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Sync, ::xx::ObjBase)
        ::xx::Shared<::SS::Scene> scene;
    };
}
namespace SS_S2C {
    struct Event : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Event, ::xx::ObjBase)
        using IsSimpleType_v = Event;
        int32_t frameNumber = 0;
        ::SS::ControlState cs;
        static void WriteTo(xx::Data& d, int32_t const& frameNumber, ::SS::ControlState const& cs);
    };
}
namespace SS_C2S {
    struct Cmd : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Cmd, ::xx::ObjBase)
        using IsSimpleType_v = Cmd;
        ::SS::ControlState cs;
        static void WriteTo(xx::Data& d, ::SS::ControlState const& cs);
    };
}
namespace xx {
	XX_OBJ_STRUCT_TEMPLATE_H(::SS::XY)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::SS::XY, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)-1).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)-1).Read(d, out); }
    };
	XX_OBJ_STRUCT_TEMPLATE_H(::SS::ControlState)
    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<::SS::ControlState, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) { (*(xx::ObjManager*)-1).Write(d, in); }
		static inline int Read(Data_r& d, T& out) { return (*(xx::ObjManager*)-1).Read(d, out); }
    };
}
#include <ss_.h.inc>
