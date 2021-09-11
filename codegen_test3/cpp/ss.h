#pragma once
#include <xx_obj.h>
#include <ss.h.inc>
struct CodeGen_ss {
	inline static const ::std::string md5 = "#*MD5<e48eef38c0a68a5a5a354a79c62f2d97>*#";
    static void Register();
    CodeGen_ss() { Register(); }
};
inline CodeGen_ss __CodeGen_ss;
namespace SS { struct Scene; }
namespace SS { struct Shooter; }
namespace SS { struct Bullet; }
namespace SS_S2C { struct EnterResult; }
namespace SS_S2C { struct Sync; }
namespace SS_S2C { struct Event; }
namespace SS_C2S { struct Enter; }
namespace SS_C2S { struct Cmd; }
namespace SS { struct Bullet_Straight; }
namespace SS { struct Bullet_Track; }
namespace xx {
    template<> struct TypeId<::SS::Scene> { static const uint16_t value = 10; };
    template<> struct TypeId<::SS::Shooter> { static const uint16_t value = 20; };
    template<> struct TypeId<::SS::Bullet> { static const uint16_t value = 30; };
    template<> struct TypeId<::SS_S2C::EnterResult> { static const uint16_t value = 50; };
    template<> struct TypeId<::SS_S2C::Sync> { static const uint16_t value = 51; };
    template<> struct TypeId<::SS_S2C::Event> { static const uint16_t value = 52; };
    template<> struct TypeId<::SS_C2S::Enter> { static const uint16_t value = 40; };
    template<> struct TypeId<::SS_C2S::Cmd> { static const uint16_t value = 41; };
    template<> struct TypeId<::SS::Bullet_Straight> { static const uint16_t value = 31; };
    template<> struct TypeId<::SS::Bullet_Track> { static const uint16_t value = 32; };
}

namespace SS {
    // 坐标
    struct XY {
        XX_OBJ_STRUCT_H(XY)
        using IsSimpleType_v = XY;
#include <ss_SSXY.inc>
        int32_t x = 0;
        int32_t y = 0;
    };
}
namespace SS {
    // 射击者的控制指令面板
    struct ControlState {
        XX_OBJ_STRUCT_H(ControlState)
        using IsSimpleType_v = ControlState;
        // 鼠标坐标( 位于场景容器中的 )
        ::SS::XY aimPos;
        // 左移
        bool moveLeft = false;
        // 右移
        bool moveRight = false;
        // 上移
        bool moveUp = false;
        // 下移
        bool moveDown = false;
        // 按钮1
        bool button1 = false;
        // 按钮2
        bool button2 = false;
    };
}
namespace SS {
    // 场景
    struct Scene : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Scene, ::xx::ObjBase)
#include <ss_SSScene.inc>
        // 帧编号
        int32_t frameNumber = 0;
        // 射击者集合. key: clientId
        ::std::map<uint32_t, ::xx::Shared<::SS::Shooter>> shooters;
    };
}
namespace SS {
    // 射击者
    struct Shooter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Shooter, ::xx::ObjBase)
#include <ss_SSShooter.inc>
        // 指向所在场景
        ::xx::Weak<::SS::Scene> scene;
        // 客户端编号( 连上服务器时自增分配 )
        uint32_t clientId = 0;
        // 身体旋转角度( 并不影响逻辑 )
        int32_t bodyAngle = 0;
        // 移动速度( 每帧移动距离 )
        int32_t speed = 10;
        // 中心点 锚点 位于 场景容器 坐标
        ::SS::XY pos;
        // 拥有的子弹集合
        ::std::vector<::xx::Shared<::SS::Bullet>> bullets;
        // 控制指令面板
        ::SS::ControlState cs;
    };
}
namespace SS {
    // 射击者的子弹( 基类 )
    struct Bullet : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Bullet, ::xx::ObjBase)
#include <ss_SSBullet.inc>
        // 指向发射者
        ::xx::Weak<::SS::Shooter> shooter;
        // 移动速度( 每帧移动距离 )
        int32_t speed = 60;
        // 剩余生命周期( 多少帧后消亡 )
        int32_t life = 0;
        // 中心点 锚点 位于 场景容器 坐标
        ::SS::XY pos;
    };
}
namespace SS_S2C {
    // 进入结果( 这应该是客户端发 Enter 之后，收到的 首包 )
    struct EnterResult : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(EnterResult, ::xx::ObjBase)
        using IsSimpleType_v = EnterResult;
        // 客户端编号, 存下来用于在 Sync.scene.shooters 中查找自身
        uint32_t clientId = 0;
        static void WriteTo(xx::Data& d, uint32_t const& clientId);
    };
}
namespace SS_S2C {
    // 完整同步
    struct Sync : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Sync, ::xx::ObjBase)
        // 整个场景数据( 后于 EnterResult 收到，但先于所有 Event )
        ::xx::Shared<::SS::Scene> scene;
    };
}
namespace SS_S2C {
    // 事件通知
    struct Event : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Event, ::xx::ObjBase)
        // 对应的帧编号( 客户端收到后，如果慢于它就需要快进，快于它就需要回滚 )
        int32_t frameNumber = 0;
        // 所有退出的玩家id列表. 客户端收到后遍历并从 shooters 中移除相应对象( 优先级:1 )
        ::std::vector<uint32_t> quits;
        // 所有进入的玩家的数据. 客户端收到后遍历挪进 shooters 并初始化( 优先级:2 )
        ::std::vector<::xx::Shared<::SS::Shooter>> enters;
        // 所有发生控制行为的玩家的 控制指令. uint: 用来定位 shooter 的 clientId
        ::std::vector<::std::tuple<uint32_t, ::SS::ControlState>> css;
    };
}
namespace SS_C2S {
    // 请求进入游戏
    struct Enter : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Enter, ::xx::ObjBase)
        using IsSimpleType_v = Enter;
        static void WriteTo(xx::Data& d);
    };
}
namespace SS_C2S {
    // 控制指令
    struct Cmd : ::xx::ObjBase {
        XX_OBJ_OBJECT_H(Cmd, ::xx::ObjBase)
        using IsSimpleType_v = Cmd;
        ::SS::ControlState cs;
        static void WriteTo(xx::Data& d, ::SS::ControlState const& cs);
    };
}
namespace SS {
    // 射击者的子弹--直线飞行
    struct Bullet_Straight : ::SS::Bullet {
        XX_OBJ_OBJECT_H(Bullet_Straight, ::SS::Bullet)
#include <ss_SSBullet_Straight.inc>
        // 每帧的 移动增量( 直线飞行专用, 已预乘 speed )
        ::SS::XY inc;
    };
}
namespace SS {
    // 射击者的子弹--跟踪
    struct Bullet_Track : ::SS::Bullet {
        XX_OBJ_OBJECT_H(Bullet_Track, ::SS::Bullet)
#include <ss_SSBullet_Track.inc>
        // 跟踪目标
        ::xx::Weak<::SS::Shooter> target;
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
