/*
    针对 adxe engine 的纯 cpp 导出项目的 lua 扩展
 
    用法：
配置 include 查找路径, 类似 $(SolutionDir)../../xxlib/lua-5.4.x/src;$(SolutionDir)../../xxlib/3rd;$(SolutionDir)../../xxlib/src;$(IncludePath)
配置项目宏 ONELUA_MAKE_LIB

    在 AppDelegate.cpp 第 34 行，也就是 一堆 include 之后，USING_NS_CC 之前 插入下列 include 语句
#include "extensions/GUI/CCScrollView/CCScrollView.h"
#include <xx_lua_adxe_binds.h>

    并于 applicationDidFinishLaunching 的最后 执行 luaBinds(this);
void AppDelegate::applicationDidEnterBackground() { 里面 添加 if (_enterBackground) _enterBackground();
void AppDelegate::applicationWillEnterForeground() { 里面 添加 if (_enterForeground) _enterForeground();
// 也可以模拟下面代码的 try 机制包裹 XX_LUA_ENABLE_TRY_CALL_FUNC

    进一步的，可去 base/ccConfig.h 改宏，关闭一些特性, 将值修改为 0
脚本绑定: CC_ENABLE_SCRIPT_BINDING   ( 必关，有本 lua 绑定方案，不再需要 cpp 那边侵入式的脚本支持 )
2d物理：CC_USE_PHYSICS
3d物理：CC_USE_3D_PHYSICS
3d导航: CC_USE_NAVMESH
3d隐面剔除: CC_USE_CULLING   ( 谨慎，这个关闭后估计所有 3d 显示就不正常了 )

    由于工作量巨大，先提供能做游戏的最小集合映射。
原则上 所有获取单例的函数，都不映射。单例的成员函数，直接映射为全局 cc_ 函数。
当前已映射的类型有：

Vec2 / 3 / 4, Color3 / 4B
Ref, Cloneable, Animation / Action 系列( 不全 )
Touch, Event / EventListener 系列
Node, Scene, Sprite, Label
ScrollView( 这个其实可以自己用 ClippingNode + EventListener 来实现 )
ClippingNode, ClippingRectangleNode
// todo:
DrawNode ...
...

*/

#include <xx_lua_data.h>
#include <xx_bufholder.h>
namespace XL = xx::Lua;

// 标识是否启用 c++ call lua 的 try 功能。开启后如果 lua 执行错误，将被捕获并打印，不会导致程序 crash
// 当前策略是仅在 debug 模式下开启. 如有需要可以改为始终开启
#ifndef NDEBUG
#   define XX_LUA_ENABLE_TRY_CALL_FUNC 1
#else
#   define XX_LUA_ENABLE_TRY_CALL_FUNC 0
#endif


// 单例缓存 & 临时数据区
inline static AppDelegate* _appDelegate = nullptr;
inline static cocos2d::AnimationCache* _animationCache = nullptr;
inline static cocos2d::SpriteFrameCache* _spriteFrameCache = nullptr;
inline static cocos2d::FileUtils* _fileUtils = nullptr;
inline static cocos2d::Director* _director = nullptr;
inline static cocos2d::GLView* _glView = nullptr;                                   // lazy init
inline static cocos2d::ActionManager* _actionManager = nullptr;
inline static cocos2d::Scheduler* _scheduler = nullptr;
inline static cocos2d::EventDispatcher* _eventDispatcher = nullptr;

inline static lua_State* _luaState = nullptr;

inline static XL::Func _globalUpdate;                                               // 注意：restart 功能实现时需先 Reset
inline static XL::Func _enterBackground;                                            // 注意：restart 功能实现时需先 Reset
inline static XL::Func _enterForeground;                                            // 注意：restart 功能实现时需先 Reset

inline static std::string _string;                                                  // for print
inline static std::vector<std::string> _strings;                                    // tmp
inline static std::vector<cocos2d::FiniteTimeAction*> _finiteTimeActions;           // tmp
inline static xx::BufHolder _bh;                                                    // tmp
inline static cocos2d::Vector<cocos2d::FiniteTimeAction*> _finiteTimeActions2;      // tmp，因为会自动加持，须退出函数时清空
inline static cocos2d::Vector<cocos2d::SpriteFrame*> _spriteFrames2;                // tmp，因为会自动加持，须退出函数时清空
inline static cocos2d::Vector<cocos2d::AnimationFrame*> _animationFrames2;          // tmp，因为会自动加持，须退出函数时清空

// 增加 cocos2d::Vector 的识别能力
namespace xx {
    template<typename T>
    struct IsCocosVector : std::false_type {
        using value_type = void;
    };
    template<typename T>
    struct IsCocosVector<cocos2d::Vector<T>> : std::true_type {
        using value_type = T;
    };
    template<typename T>
    struct IsCocosVector<cocos2d::Vector<T>&> : std::true_type {
        using value_type = T;
    };
    template<typename T>
    struct IsCocosVector<cocos2d::Vector<T> const&> : std::true_type {
        using value_type = T;
    };
    template<typename T>
    constexpr bool IsCocosVector_v = IsCocosVector<T>::value;
    template<typename T>
    using CocosVector_vt = IsCocosVector<T>::value_type;
}

// 模板适配区
namespace xx::Lua {
    /*******************************************************************************************/
    // 值类型直接映射到 push + to

    // 适配 cocos2d::Vec2/3/4. 体现为 table 数组 ( copy )
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<
        std::is_same_v<cocos2d::Vec2, std::decay_t<T>>
        || std::is_same_v<cocos2d::Vec3, std::decay_t<T>>
        || std::is_same_v<cocos2d::Vec4, std::decay_t<T>>
        >> {
        using U = std::decay_t<T>;
        static int Push(lua_State* const& L, T&& in) {
            CheckStack(L, 3);
            lua_createtable(L, sizeof(U) / 4, 0);
            SetField(L, 1, in.x);
            SetField(L, 2, in.y);
            if constexpr (sizeof(U) > 8) {
                SetField(L, 3, in.z);
            }
            if constexpr (sizeof(U) > 12) {
                SetField(L, 4, in.w);
            }
            return 1;
        }
        static void To(lua_State* const& L, int const& idx, T& out) {
            if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", std::string(xx::TypeName_v<U>));
            int vpos = lua_gettop(L) + 1;
            CheckStack(L, 3);
            // x
            lua_rawgeti(L, idx, 1);									// ... t(idx), ..., val/nil
            if (lua_isnil(L, vpos)) {
                lua_pop(L, 1);										// ... t(idx), ...
                return;
            }
            ::xx::Lua::To(L, vpos, out.x);
            lua_pop(L, 1);											// ... t(idx), ...
            // y
            lua_rawgeti(L, idx, 2);									// ... t(idx), ..., val/nil
            if (lua_isnil(L, vpos)) {
                lua_pop(L, 1);										// ... t(idx), ...
                return;
            }
            ::xx::Lua::To(L, vpos, out.y);
            lua_pop(L, 1);											// ... t(idx), ...
            // z
            if constexpr (sizeof(U) > 8) {
                lua_rawgeti(L, idx, 3);									// ... t(idx), ..., val/nil
                if (lua_isnil(L, vpos)) {
                    lua_pop(L, 1);										// ... t(idx), ...
                    return;
                }
                ::xx::Lua::To(L, vpos, out.z);
                lua_pop(L, 1);											// ... t(idx), ...
            }
            // w
            if constexpr (sizeof(U) > 12) {
                lua_rawgeti(L, idx, 4);									// ... t(idx), ..., val/nil
                if (lua_isnil(L, vpos)) {
                    lua_pop(L, 1);										// ... t(idx), ...
                    return;
                }
                ::xx::Lua::To(L, vpos, out.w);
                lua_pop(L, 1);											// ... t(idx), ...
            }
            assert(vpos = lua_gettop(L) + 1);
        }
    };

    // 适配 cocos2d::Color3/4B. 体现为 table 数组 ( copy )
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<
        std::is_same_v<cocos2d::Color3B, std::decay_t<T>>
        || std::is_same_v<cocos2d::Color4B, std::decay_t<T>>
        >> {
        using U = std::decay_t<T>;
        static int Push(lua_State* const& L, T&& in) {
            CheckStack(L, 3);
            lua_createtable(L, 4, 0);
            SetField(L, 1, in.r);
            SetField(L, 2, in.g);
            SetField(L, 3, in.b);
            if constexpr (std::is_same_v<cocos2d::Color4B, U>) {
                SetField(L, 4, in.a);
            }
            return 1;
        }
        static void To(lua_State* const& L, int const& idx, T& out) {
            if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", std::string(xx::TypeName_v<U>));
            int vpos = lua_gettop(L) + 1;
            CheckStack(L, 3);
            // r
            lua_rawgeti(L, idx, 1);									// ... t(idx), ..., val/nil
            if (lua_isnil(L, vpos)) {
                lua_pop(L, 1);										// ... t(idx), ...
                return;
            }
            ::xx::Lua::To(L, vpos, out.r);
            lua_pop(L, 1);											// ... t(idx), ...
            // g
            lua_rawgeti(L, idx, 2);									// ... t(idx), ..., val/nil
            if (lua_isnil(L, vpos)) {
                lua_pop(L, 1);										// ... t(idx), ...
                return;
            }
            ::xx::Lua::To(L, vpos, out.g);
            lua_pop(L, 1);											// ... t(idx), ...
            // b
            lua_rawgeti(L, idx, 3);									// ... t(idx), ..., val/nil
            if (lua_isnil(L, vpos)) {
                lua_pop(L, 1);										// ... t(idx), ...
                return;
            }
            ::xx::Lua::To(L, vpos, out.b);
            lua_pop(L, 1);											// ... t(idx), ...
            // a
            if constexpr (std::is_same_v<cocos2d::Color4B, U>) {
                lua_rawgeti(L, idx, 4);									// ... t(idx), ..., val/nil
                if (lua_isnil(L, vpos)) {
                    lua_pop(L, 1);										// ... t(idx), ...
                    return;
                }
                ::xx::Lua::To(L, vpos, out.a);
                lua_pop(L, 1);											// ... t(idx), ...
            }
            assert(vpos = lua_gettop(L) + 1);
        }
    };

    // 适配 cocos2d::Vector<T>
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<IsCocosVector_v<std::decay_t<T>>>> {
        static int Push(lua_State* const& L, T&& in) {
            CheckStack(L, 3);
            auto siz = (int)in.size();
            lua_createtable(L, siz, 0);
            for (int i = 0; i < siz; ++i) {
                SetField(L, i + 1, in.at(i));
            }
            return 1;
        }
        static void To(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
            if (!lua_istable(L, idx)) Error(L, "error! args[", std::to_string(idx), "] is not table:", std::string(xx::TypeName_v<T>));
#endif
            out.clear();
            int top = lua_gettop(L) + 1;
            CheckStack(L, 3);
            out.reserve(32);
            for (lua_Integer i = 1;; i++) {
                lua_rawgeti(L, idx, i);									// ... t(idx), ..., val/nil
                if (lua_isnil(L, top)) {
                    lua_pop(L, 1);										// ... t(idx), ...
                    return;
                }
                CocosVector_vt<std::decay_t<T>> v;
                ::xx::Lua::To(L, top, v);
                lua_pop(L, 1);											// ... t(idx), ...
                out.pushBack(std::move(v));
            }
        }
    };


    /*******************************************************************************************/
    // Ref
    template<>
    struct MetaFuncs<cocos2d::Ref*, void> {
        using U = cocos2d::Ref*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            // MetaFuncs<BaseType*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "retain", [](auto L)->int { To<U>(L)->retain(); return 0; });
            SetFieldCClosure(L, "release", [](auto L)->int { To<U>(L)->release(); return 0; });
            SetFieldCClosure(L, "autorelease", [](auto L)->int { To<U>(L)->autorelease(); return 0; });
            SetFieldCClosure(L, "getReferenceCount", [](auto L)->int { return Push(L, To<U>(L)->getReferenceCount()); });
        }
    };

    /*******************************************************************************************/
    // Clonable
    // 这个类的子类，需要自己实现返回具体类型的 clone 函数。直接压 Cloneable* 到 lua 没啥用，Push 层无法附加正确的 meta
    // 类似的情况还有 Action::reverse， 需要实现到具体派生类, 类似下面的代码
    // SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); });   // override

    /*******************************************************************************************/
    // Touch : Ref
    template<>
    struct MetaFuncs<cocos2d::Touch*, void> {
        using U = cocos2d::Touch*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getLocation", [](auto L)->int {
                auto&& r = To<U>(L)->getLocation();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getPreviousLocation", [](auto L)->int {
                auto&& r = To<U>(L)->getPreviousLocation();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getStartLocation", [](auto L)->int {
                auto&& r = To<U>(L)->getStartLocation();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getDelta", [](auto L)->int {
                auto&& r = To<U>(L)->getDelta();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getLocationInView", [](auto L)->int {
                auto&& r = To<U>(L)->getLocationInView();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getPreviousLocationInView", [](auto L)->int {
                auto&& r = To<U>(L)->getPreviousLocationInView();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getStartLocationInView", [](auto L)->int {
                auto&& r = To<U>(L)->getStartLocationInView();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "setTouchInfo", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 4: {
                    To<U>(L)->setTouchInfo(To<int>(L, 2), To<float>(L, 3), To<float>(L, 4)); return 0;
                }
                case 6: {
                    To<U>(L)->setTouchInfo(To<int>(L, 2), To<float>(L, 3), To<float>(L, 4), To<float>(L, 5), To<float>(L, 6)); return 0;
                }
                default:
                    return luaL_error(L, "%s", "setTouchInfo error! need 4 / 6 args: self, int id, float x, float y, float force, float maxForce");
                }
            });
            SetFieldCClosure(L, "getID", [](auto L)->int { return Push(L, To<U>(L)->getID()); });
            SetFieldCClosure(L, "getCurrentForce", [](auto L)->int { return Push(L, To<U>(L)->getCurrentForce()); });
            SetFieldCClosure(L, "getMaxForce", [](auto L)->int { return Push(L, To<U>(L)->getMaxForce()); });
        }
    };

    /*******************************************************************************************/
    // Animation : Ref, Clonable
    template<>
    struct MetaFuncs<cocos2d::Animation*, void> {
        using U = cocos2d::Animation*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "addSpriteFrame", [](auto L)->int { To<U>(L)->addSpriteFrame(To<cocos2d::SpriteFrame*>(L, 2)); return 0; });
            SetFieldCClosure(L, "addSpriteFrameWithFile", [](auto L)->int { To<U>(L)->addSpriteFrameWithFile(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "addSpriteFrameWithTexture", [](auto L)->int { To<U>(L)->addSpriteFrameWithTexture(To<cocos2d::Texture2D*>(L, 2)
                , { To<float>(L, 3), To<float>(L, 4), To<float>(L, 5), To<float>(L, 6) }); return 0; });
            SetFieldCClosure(L, "getTotalDelayUnits", [](auto L)->int { return Push(L, To<U>(L)->getTotalDelayUnits()); });
            SetFieldCClosure(L, "setDelayPerUnit", [](auto L)->int { To<U>(L)->setDelayPerUnit(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getDelayPerUnit", [](auto L)->int { return Push(L, To<U>(L)->getDelayPerUnit()); });
            SetFieldCClosure(L, "getDuration", [](auto L)->int { return Push(L, To<U>(L)->getDuration()); });
            SetFieldCClosure(L, "getFrames", [](auto L)->int { return Push(L, To<U>(L)->getFrames()); });
            SetFieldCClosure(L, "setFrames", [](auto L)->int {
                auto&& sg = xx::MakeScopeGuard([&] { _animationFrames2.clear(); });
                To(L, 2, _animationFrames2);
                To<U>(L)->setFrames(_animationFrames2);
                return 0;
            });
            SetFieldCClosure(L, "getRestoreOriginalFrame", [](auto L)->int { return Push(L, To<U>(L)->getRestoreOriginalFrame()); });
            SetFieldCClosure(L, "setRestoreOriginalFrame", [](auto L)->int { To<U>(L)->setRestoreOriginalFrame(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "getLoops", [](auto L)->int { return Push(L, To<U>(L)->getLoops()); });
            SetFieldCClosure(L, "setLoops", [](auto L)->int { To<U>(L)->setLoops(To<uint32_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); });   // override
        }
    };
    /*******************************************************************************************/
    // Action : Ref, Clonable
    template<>
    struct MetaFuncs<cocos2d::Action*, void> {
        using U = cocos2d::Action*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "description", [](auto L)->int { return Push(L, To<U>(L)->description()); });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); });
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); });
            SetFieldCClosure(L, "isDone", [](auto L)->int { return Push(L, To<U>(L)->isDone()); });
            SetFieldCClosure(L, "startWithTarget", [](auto L)->int { To<U>(L)->startWithTarget(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "stop", [](auto L)->int { To<U>(L)->stop(); return 0; });
            SetFieldCClosure(L, "step", [](auto L)->int { To<U>(L)->step(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "update", [](auto L)->int { To<U>(L)->update(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getTarget", [](auto L)->int { return Push(L, To<U>(L)->getTarget()); });
            SetFieldCClosure(L, "setTarget", [](auto L)->int { To<U>(L)->setTarget(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getOriginalTarget", [](auto L)->int { return Push(L, To<U>(L)->getOriginalTarget()); });
            SetFieldCClosure(L, "setOriginalTarget", [](auto L)->int { To<U>(L)->setOriginalTarget(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getTag", [](auto L)->int { return Push(L, To<U>(L)->getTag()); });
            SetFieldCClosure(L, "setTag", [](auto L)->int { To<U>(L)->setTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getFlags", [](auto L)->int { return Push(L, To<U>(L)->getFlags()); });
            SetFieldCClosure(L, "setFlags", [](auto L)->int { To<U>(L)->setFlags(To<uint32_t>(L, 2)); return 0; });
        }
    };
    // FiniteTimeAction : Action
    template<>
    struct MetaFuncs<cocos2d::FiniteTimeAction*, void> {
        using U = cocos2d::FiniteTimeAction*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Action*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getDuration", [](auto L)->int { return Push(L, To<U>(L)->getDuration()); });
            SetFieldCClosure(L, "setDuration", [](auto L)->int { To<U>(L)->setDuration(To<float>(L, 2)); return 0; });
        }
    };
    // ActionInterval : FiniteTimeAction
    template<>
    struct MetaFuncs<cocos2d::ActionInterval*, void> {
        using U = cocos2d::ActionInterval*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::FiniteTimeAction*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getElapsed", [](auto L)->int { return Push(L, To<U>(L)->getElapsed()); });
            SetFieldCClosure(L, "setAmplitudeRate", [](auto L)->int { To<U>(L)->setAmplitudeRate(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getAmplitudeRate", [](auto L)->int { return Push(L, To<U>(L)->getAmplitudeRate()); });
        }
    };
    // ActionInstant : FiniteTimeAction
    template<>
    struct MetaFuncs<cocos2d::ActionInstant*, void> {
        using U = cocos2d::ActionInstant*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::FiniteTimeAction*>::Fill(L);
            SetType<U>(L);
        }
    };
    // Animate : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::Animate*, void> {
        using U = cocos2d::Animate*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setAnimation", [](auto L)->int { To<U>(L)->setAnimation(To<cocos2d::Animation*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getAnimation", [](auto L)->int { return Push(L, To<U>(L)->getAnimation()); });
            SetFieldCClosure(L, "getCurrentFrameIndex", [](auto L)->int { return Push(L, To<U>(L)->getCurrentFrameIndex()); });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };

    // BezierBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::BezierBy*, void> {
        using U = cocos2d::BezierBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // BezierTo : BezierBy
    template<>
    struct MetaFuncs<cocos2d::BezierTo*, void> {
        using U = cocos2d::BezierTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::BezierBy*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Blink : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::Blink*, void> {
        using U = cocos2d::Blink*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // CallFunc : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::CallFunc*, void> {
        using U = cocos2d::CallFunc*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // DelayTime : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::DelayTime*, void> {
        using U = cocos2d::DelayTime*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };

    // FadeTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::FadeTo*, void> {
        using U = cocos2d::FadeTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // FadeIn : FadeTo
    template<>
    struct MetaFuncs<cocos2d::FadeIn*, void> {
        using U = cocos2d::FadeIn*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::FadeTo*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // FadeOut : FadeTo
    template<>
    struct MetaFuncs<cocos2d::FadeOut*, void> {
        using U = cocos2d::FadeOut*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::FadeTo*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // FlipX : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::FlipX*, void> {
        using U = cocos2d::FlipX*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // FlipY : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::FlipY*, void> {
        using U = cocos2d::FlipY*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Hide : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::Hide*, void> {
        using U = cocos2d::Hide*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // JumpBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::JumpBy*, void> {
        using U = cocos2d::JumpBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // JumpTo : JumpBy
    template<>
    struct MetaFuncs<cocos2d::JumpTo*, void> {
        using U = cocos2d::JumpTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::JumpBy*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // MoveBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::MoveBy*, void> {
        using U = cocos2d::MoveBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // MoveTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::MoveTo*, void> {
        using U = cocos2d::MoveTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Place : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::Place*, void> {
        using U = cocos2d::Place*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // RemoveSelf : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::RemoveSelf*, void> {
        using U = cocos2d::RemoveSelf*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Repeat : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::Repeat*, void> {
        using U = cocos2d::Repeat*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setInnerAction", [](auto L)->int { To<U>(L)->setInnerAction(To<cocos2d::FiniteTimeAction*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getInnerAction", [](auto L)->int { return Push(L, To<U>(L)->getInnerAction()); });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // RepeatForever : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::RepeatForever*, void> {
        using U = cocos2d::RepeatForever*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getInnerAction", [](auto L)->int { return Push(L, To<U>(L)->getInnerAction()); });
            SetFieldCClosure(L, "setInnerAction", [](auto L)->int { To<U>(L)->setInnerAction(To<cocos2d::ActionInterval*>(L, 2)); return 0; });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // ResizeBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::ResizeBy*, void> {
        using U = cocos2d::ResizeBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // ResizeTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::ResizeTo*, void> {
        using U = cocos2d::ResizeTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            // todo: cocos 代码这里没有覆盖 reverse
        }
    };
    // RotateBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::RotateBy*, void> {
        using U = cocos2d::RotateBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // RotateTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::RotateTo*, void> {
        using U = cocos2d::RotateTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // ScaleTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::ScaleTo*, void> {
        using U = cocos2d::ScaleTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // ScaleBy : ScaleTo
    template<>
    struct MetaFuncs<cocos2d::ScaleBy*, void> {
        using U = cocos2d::ScaleBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ScaleTo*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Sequence : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::Sequence*, void> {
        using U = cocos2d::Sequence*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Show : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::Show*, void> {
        using U = cocos2d::Show*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // SkewTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::SkewTo*, void> {
        using U = cocos2d::SkewTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // SkewBy : SkewTo
    template<>
    struct MetaFuncs<cocos2d::SkewBy*, void> {
        using U = cocos2d::SkewBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::SkewTo*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Spawn : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::Spawn*, void> {
        using U = cocos2d::Spawn*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // TintTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::TintTo*, void> {
        using U = cocos2d::TintTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // TintBy : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::TintBy*, void> {
        using U = cocos2d::TintBy*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // ToggleVisibility : ActionInstant
    template<>
    struct MetaFuncs<cocos2d::ToggleVisibility*, void> {
        using U = cocos2d::ToggleVisibility*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInstant*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };
    // Speed : Action
    template<>
    struct MetaFuncs<cocos2d::Speed*, void> {
        using U = cocos2d::Speed*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Action*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getSpeed", [](auto L)->int { return Push(L, To<U>(L)->getSpeed()); });
            SetFieldCClosure(L, "setSpeed", [](auto L)->int { To<U>(L)->setSpeed(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getInnerAction", [](auto L)->int { return Push(L, To<U>(L)->getInnerAction()); });
            SetFieldCClosure(L, "setInnerAction", [](auto L)->int { To<U>(L)->setInnerAction(To<cocos2d::ActionInterval*>(L, 2)); return 0; });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
            SetFieldCClosure(L, "reverse", [](auto L)->int { return Push(L, (U)To<U>(L)->reverse()); }); // override
        }
    };

    // todo: more action?



    /*******************************************************************************************/
    // Event : Ref
    template<>
    struct MetaFuncs<cocos2d::Event*, void> {
        using U = cocos2d::Event*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getType", [](auto L)->int { return Push(L, To<U>(L)->getType()); });
            SetFieldCClosure(L, "stopPropagation", [](auto L)->int { To<U>(L)->stopPropagation(); return 0; });
            SetFieldCClosure(L, "isStopped", [](auto L)->int { return Push(L, To<U>(L)->isStopped()); });
            SetFieldCClosure(L, "getCurrentTarget", [](auto L)->int { return Push(L, To<U>(L)->getCurrentTarget()); });
        }
    };

    // EventCustom : Event
    template<>
    struct MetaFuncs<cocos2d::EventCustom*, void> {
        using U = cocos2d::EventCustom*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Event*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setUserData", [](auto L)->int { To<U>(L)->setUserData(To<void*>(L, 1));  return 0; });
            SetFieldCClosure(L, "getUserData", [](auto L)->int { return Push(L, To<U>(L)->getUserData()); });
            SetFieldCClosure(L, "getEventName", [](auto L)->int { return Push(L, To<U>(L)->getEventName()); });
        }
    };

    // EventListener : Ref
    template<>
    struct MetaFuncs<cocos2d::EventListener*, void> {
        using U = cocos2d::EventListener*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); });
            SetFieldCClosure(L, "checkAvailable", [](auto L)->int { return Push(L, To<U>(L)->checkAvailable()); });
            SetFieldCClosure(L, "setEnabled", [](auto L)->int { To<U>(L)->setEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isEnabled", [](auto L)->int { return Push(L, To<U>(L)->isEnabled()); });
        }
    };

    // EventListenerTouchAllAtOnce : EventListener
    template<>
    struct MetaFuncs<cocos2d::EventListenerTouchAllAtOnce*, void> {
        using U = cocos2d::EventListenerTouchAllAtOnce*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::EventListener*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "onTouchesBegan", [](auto L)->int {
                To<U>(L)->onTouchesBegan = [f = To<Func>(L, 2)](const std::vector<cocos2d::Touch*>& ts, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(ts, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesMoved", [](auto L)->int {
                To<U>(L)->onTouchesMoved = [f = To<Func>(L, 2)](const std::vector<cocos2d::Touch*>& ts, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(ts, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesEnded", [](auto L)->int {
                To<U>(L)->onTouchesEnded = [f = To<Func>(L, 2)](const std::vector<cocos2d::Touch*>& ts, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(ts, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesCancelled", [](auto L)->int {
                To<U>(L)->onTouchesCancelled = [f = To<Func>(L, 2)](const std::vector<cocos2d::Touch*>& ts, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(ts, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
        }
    };

    // EventListenerTouchOneByOne : EventListener
    template<>
    struct MetaFuncs<cocos2d::EventListenerTouchOneByOne*, void> {
        using U = cocos2d::EventListenerTouchOneByOne*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::EventListener*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setSwallowTouches", [](auto L)->int { To<U>(L)->setSwallowTouches(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isSwallowTouches", [](auto L)->int { return Push(L, To<U>(L)->isSwallowTouches()); });
            SetFieldCClosure(L, "onTouchesBegan", [](auto L)->int {
                To<U>(L)->onTouchBegan = [f = To<Func>(L, 2)](cocos2d::Touch* t, cocos2d::Event* e)->bool {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        return f.Call<bool>(t, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
                    return false;
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesMoved", [](auto L)->int {
                To<U>(L)->onTouchMoved = [f = To<Func>(L, 2)](cocos2d::Touch* t, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(t, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesEnded", [](auto L)->int {
                To<U>(L)->onTouchEnded = [f = To<Func>(L, 2)](cocos2d::Touch* t, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(t, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onTouchesCancelled", [](auto L)->int {
                To<U>(L)->onTouchCancelled = [f = To<Func>(L, 2)](cocos2d::Touch* t, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(t, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
        }
    };
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    // EventListenerKeyboard : EventListener
    template<>
    struct MetaFuncs<cocos2d::EventListenerKeyboard*, void> {
        using U = cocos2d::EventListenerKeyboard*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::EventListener*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "onKeyPressed", [](auto L)->int {
                To<U>(L)->onKeyPressed = [f = To<Func>(L, 2)](cocos2d::EventKeyboard::KeyCode k, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(k, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "onKeyReleased", [](auto L)->int {
                To<U>(L)->onKeyReleased = [f = To<Func>(L, 2)](cocos2d::EventKeyboard::KeyCode k, cocos2d::Event* e)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(k, e);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                };
                return 0;
            });
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (U)To<U>(L)->clone()); }); // override
        }
    };

    // todo: more EventListener?

    /*******************************************************************************************/
    // Component : Ref
    template<>
    struct MetaFuncs<cocos2d::Component*, void> {
        using U = cocos2d::Component*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    // PhysicsBody : Component
    template<>
    struct MetaFuncs<cocos2d::PhysicsBody*, void> {
        using U = cocos2d::PhysicsBody*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Component*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // Node : Ref
    template<>
    struct MetaFuncs<cocos2d::Node*, void> {
        using U = cocos2d::Node*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            // 针对 基类向派生类转换的需求，提供一组 asType 的函数给 lua，以便附加正确的 meta
            SetFieldCClosure(L, "toScene", [](auto L)->int { return Push(L, To<cocos2d::Scene*>(L)); });
            SetFieldCClosure(L, "toSprite", [](auto L)->int { return Push(L, To<cocos2d::Sprite*>(L)); });
            // todo: more asXxxx
            SetFieldCClosure(L, "getDescription", [](auto L)->int { return Push(L, To<U>(L)->getDescription()); });
            SetFieldCClosure(L, "setLocalZOrder", [](auto L)->int { To<U>(L)->setLocalZOrder(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "_setLocalZOrder", [](auto L)->int { To<U>(L)->_setLocalZOrder(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "updateOrderOfArrival", [](auto L)->int { To<U>(L)->updateOrderOfArrival(); return 0; });
            SetFieldCClosure(L, "getLocalZOrder", [](auto L)->int { return Push(L, To<U>(L)->getLocalZOrder()); });
            SetFieldCClosure(L, "setGlobalZOrder", [](auto L)->int { To<U>(L)->setGlobalZOrder(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getGlobalZOrder", [](auto L)->int { return Push(L, To<U>(L)->getGlobalZOrder()); });
            SetFieldCClosure(L, "setScaleX", [](auto L)->int { To<U>(L)->setScaleX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleX", [](auto L)->int { return Push(L, To<U>(L)->getScaleX()); });
            SetFieldCClosure(L, "setScaleY", [](auto L)->int { To<U>(L)->setScaleY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleY", [](auto L)->int { return Push(L, To<U>(L)->getScaleY()); });
            SetFieldCClosure(L, "setScaleZ", [](auto L)->int { To<U>(L)->setScaleZ(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleZ", [](auto L)->int { return Push(L, To<U>(L)->getScaleZ()); });
            SetFieldCClosure(L, "setScale", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setScale(To<float>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->setScale(To<float>(L, 2), To<float>(L, 3));
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "setScale error! need 2 ~ 3 args: self, float scaleX+Y / scaleX, scaleY");
                }
            });
            SetFieldCClosure(L, "getScale", [](auto L)->int { return Push(L, To<U>(L)->getScale()); });
            SetFieldCClosure(L, "setPosition", [](auto L)->int { To<U>(L)->setPosition(To<float>(L, 2), To<float>(L, 3)); return 0; });
            SetFieldCClosure(L, "setPositionNormalized", [](auto L)->int { To<U>(L)->setPositionNormalized({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getPosition", [](auto L)->int { auto&& o = To<U>(L)->getPosition(); return Push(L, o.x, o.y); });
            SetFieldCClosure(L, "getPositionNormalized", [](auto L)->int {
                auto&& r = To<U>(L)->getPositionNormalized();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "setPositionX", [](auto L)->int { To<U>(L)->setPositionX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionX", [](auto L)->int { return Push(L, To<U>(L)->getPositionX()); });
            SetFieldCClosure(L, "setPositionY", [](auto L)->int { To<U>(L)->setPositionY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionY", [](auto L)->int { return Push(L, To<U>(L)->getPositionY()); });
            SetFieldCClosure(L, "setPosition3D", [](auto L)->int { To<U>(L)->setPosition3D({ To<float>(L, 2), To<float>(L, 3), To<float>(L, 4) }); return 0; });
            SetFieldCClosure(L, "getPosition3D", [](auto L)->int {
                auto&& o = To<U>(L)->getPosition3D();
                return Push(L, o.x, o.y, o.z);
            });
            SetFieldCClosure(L, "setPositionZ", [](auto L)->int { To<U>(L)->setPositionZ(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionZ", [](auto L)->int { return Push(L, To<U>(L)->getPositionZ()); });
            SetFieldCClosure(L, "setSkewX", [](auto L)->int { To<U>(L)->setSkewX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getSkewX", [](auto L)->int { return Push(L, To<U>(L)->getSkewX()); });
            SetFieldCClosure(L, "setSkewY", [](auto L)->int { To<U>(L)->setSkewY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getSkewY", [](auto L)->int { return Push(L, To<U>(L)->getSkewY()); });
            SetFieldCClosure(L, "setAnchorPoint", [](auto L)->int { To<U>(L)->setAnchorPoint({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getAnchorPoint", [](auto L)->int {
                auto&& r = To<U>(L)->getAnchorPoint();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "getAnchorPointInPoints", [](auto L)->int {
                auto&& r = To<U>(L)->getAnchorPointInPoints();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "setContentSize", [](auto L)->int { To<U>(L)->setContentSize({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getContentSize", [](auto L)->int {
                auto&& r = To<U>(L)->getContentSize();
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "hitTest", [](auto L)->int { return Push(L, To<U>(L)->hitTest({ To<float>(L, 2), To<float>(L, 3) })); });
            SetFieldCClosure(L, "setVisible", [](auto L)->int { To<U>(L)->setVisible(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isVisible", [](auto L)->int { return Push(L, To<U>(L)->isVisible()); });
            SetFieldCClosure(L, "setRotation", [](auto L)->int { To<U>(L)->setRotation(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getRotation", [](auto L)->int { return Push(L, To<U>(L)->getRotation()); });
            SetFieldCClosure(L, "setRotation3D", [](auto L)->int { To<U>(L)->setRotation3D({ To<float>(L, 2), To<float>(L, 3), To<float>(L, 4) }); return 0; });
            SetFieldCClosure(L, "getRotation3D", [](auto L)->int {
                auto&& r = To<U>(L)->getRotation3D();
                return Push(L, r.x, r.y, r.z);
            });
            SetFieldCClosure(L, "setRotationQuat", [](auto L)->int { To<U>(L)->setRotationQuat({ To<float>(L, 2), To<float>(L, 3), To<float>(L, 4), To<float>(L, 5) }); return 0; });
            SetFieldCClosure(L, "getRotationQuat", [](auto L)->int {
                auto&& r = To<U>(L)->getRotationQuat();
                return Push(L, r.x, r.y, r.z, r.w);
            });
            SetFieldCClosure(L, "setRotationSkewX", [](auto L)->int { To<U>(L)->setRotationSkewX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getRotationSkewX", [](auto L)->int { return Push(L, To<U>(L)->getRotationSkewX()); });
            SetFieldCClosure(L, "setRotationSkewY", [](auto L)->int { To<U>(L)->setRotationSkewY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getRotationSkewY", [](auto L)->int { return Push(L, To<U>(L)->getRotationSkewY()); });
            SetFieldCClosure(L, "setIgnoreAnchorPointForPosition", [](auto L)->int { To<U>(L)->setIgnoreAnchorPointForPosition(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isIgnoreAnchorPointForPosition", [](auto L)->int { return Push(L, To<U>(L)->isIgnoreAnchorPointForPosition()); });
            SetFieldCClosure(L, "addChild", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->addChild(XL::To<cocos2d::Node*>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->addChild(XL::To<cocos2d::Node*>(L, 2), XL::To<int>(L, 3));
                    return 0;
                }
                case 4: {
                    if (lua_isnumber(L, 4)) {
                        To<U>(L)->addChild(XL::To<cocos2d::Node*>(L, 2), XL::To<int>(L, 3), XL::To<int>(L, 4));
                    }
                    else {
                        To<U>(L)->addChild(XL::To<cocos2d::Node*>(L, 2), XL::To<int>(L, 3), XL::To<std::string>(L, 4));
                    }
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "addChild error! need 2 ~ 4 args: self, Node child, int localZOrder, int tag/string name");
                }
            });
            SetFieldCClosure(L, "getChildByTag", [](auto L)->int { return Push(L, To<U>(L)->getChildByTag(XL::To<int>(L, 2))); });
            SetFieldCClosure(L, "getChildByName", [](auto L)->int { return Push(L, To<U>(L)->getChildByName(XL::To<std::string>(L, 2))); });
            SetFieldCClosure(L, "enumerateChildren", [](auto L)->int {
                To<U>(L)->enumerateChildren(To<std::string>(L, 2), [f = To<Func>(L, 2)](cocos2d::Node* node)->bool {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        return f.Call<bool>(node);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
                    return false;
#endif
                });
                return 0; 
            });
            SetFieldCClosure(L, "getChildren", [](auto L)->int { return Push(L, To<U>(L)->getChildren()); });
            SetFieldCClosure(L, "getChildrenCount", [](auto L)->int { return Push(L, To<U>(L)->getChildrenCount()); });
            SetFieldCClosure(L, "setParent", [](auto L)->int { To<U>(L)->setParent(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getParent", [](auto L)->int { return Push(L, To<U>(L)->getParent()); });
            SetFieldCClosure(L, "removeFromParent", [](auto L)->int { To<U>(L)->removeFromParent(); return 0; });
            SetFieldCClosure(L, "removeFromParentAndCleanup", [](auto L)->int { To<U>(L)->removeFromParentAndCleanup(XL::To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "removeChild", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->removeChild(XL::To<cocos2d::Node*>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->removeChild(XL::To<cocos2d::Node*>(L, 2), XL::To<bool>(L, 3));
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "removeChild error! need 2 ~ 3 args: self, Node child, bool cleanup = true");
                }
            });
            SetFieldCClosure(L, "removeChildByTag", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->removeChildByTag(XL::To<int>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->removeChildByTag(XL::To<int>(L, 2), XL::To<bool>(L, 3));
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "removeChildByTag error! need 2 ~ 3 args: self, int tag, bool cleanup = true");
                }
            });
            SetFieldCClosure(L, "removeChildByName", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->removeChildByName(XL::To<std::string>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->removeChildByName(XL::To<std::string>(L, 2), XL::To<bool>(L, 3));
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "removeChildByName error! need 2 ~ 3 args: self, string name, bool cleanup = true");
                }
            });
            SetFieldCClosure(L, "removeAllChildren", [](auto L)->int { To<U>(L)->removeAllChildren(); return 0; });
            SetFieldCClosure(L, "removeAllChildrenWithCleanup", [](auto L)->int { To<U>(L)->removeAllChildrenWithCleanup(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "reorderChild", [](auto L)->int { To<U>(L)->reorderChild(To<cocos2d::Node*>(L, 2), To<int>(L, 3)); return 0; });
            SetFieldCClosure(L, "sortAllChildren", [](auto L)->int { To<U>(L)->sortAllChildren(); return 0; });
            // todo: cc_sortNodes ?
            SetFieldCClosure(L, "getTag", [](auto L)->int { return Push(L, To<U>(L)->getTag()); });
            SetFieldCClosure(L, "setTag", [](auto L)->int { To<U>(L)->setTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getName", [](auto L)->int { return Push(L, To<U>(L)->getName()); });
            SetFieldCClosure(L, "setName", [](auto L)->int { To<U>(L)->setName(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "getUserData", [](auto L)->int { return Push(L, To<U>(L)->getUserData()); });
            SetFieldCClosure(L, "setUserData", [](auto L)->int { To<U>(L)->setUserData(To<void*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getUserObject", [](auto L)->int { return Push(L, To<U>(L)->getUserObject()); });
            SetFieldCClosure(L, "setUserObject", [](auto L)->int { To<U>(L)->setUserObject(To<cocos2d::Ref*>(L, 2)); return 0; });
            SetFieldCClosure(L, "isRunning", [](auto L)->int { return Push(L, To<U>(L)->isRunning()); });
            // 不实现 scheduleUpdateWithPriorityLua
            SetFieldCClosure(L, "onEnter", [](auto L)->int { To<U>(L)->onEnter(); return 0; });
            SetFieldCClosure(L, "onEnterTransitionDidFinish", [](auto L)->int { To<U>(L)->onEnterTransitionDidFinish(); return 0; });
            SetFieldCClosure(L, "onExit", [](auto L)->int { To<U>(L)->onExit(); return 0; });
            SetFieldCClosure(L, "onExitTransitionDidStart", [](auto L)->int { To<U>(L)->onExitTransitionDidStart(); return 0; });
            SetFieldCClosure(L, "cleanup", [](auto L)->int { To<U>(L)->cleanup(); return 0; });
            // todo: draw, visit
            SetFieldCClosure(L, "getScene", [](auto L)->int { return Push(L, To<U>(L)->getScene()); });
            SetFieldCClosure(L, "getBoundingBox", [](auto L)->int {
                auto&& r = To<U>(L)->getBoundingBox();
                return Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            // todo: setEventDispatcher getEventDispatcher setActionManager getActionManager
            SetFieldCClosure(L, "runAction", [](auto L)->int { return Push(L, To<U>(L)->runAction(To<cocos2d::Action*>(L, 2))); });
            SetFieldCClosure(L, "stopAllActions", [](auto L)->int { To<U>(L)->stopAllActions(); return 0; });
            SetFieldCClosure(L, "stopAction", [](auto L)->int { To<U>(L)->stopAction(To<cocos2d::Action*>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopActionByTag", [](auto L)->int { To<U>(L)->stopActionByTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopAllActionsByTag", [](auto L)->int { To<U>(L)->stopAllActionsByTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopActionsByFlags", [](auto L)->int { To<U>(L)->stopActionsByFlags(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getActionByTag", [](auto L)->int { return Push(L, To<U>(L)->getActionByTag(To<int>(L, 2))); });
            SetFieldCClosure(L, "getNumberOfRunningActions", [](auto L)->int { return Push(L, To<U>(L)->getNumberOfRunningActions()); });
            // todo: 有些函数映射到 lua 没啥意义 慢慢排查清理. 已知有 setScheduler getScheduler isScheduled  scheduleUpdateWithPriority
            // 这个函数因为 lua 无法 override update 而修改，利用 schedule 来实现类似效果。key 使用指针值来生成
            SetFieldCClosure(L, "scheduleUpdate", [](auto L)->int {
                To<U>(L)->schedule([f = To<Func>(L, 2)](float delta) {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(delta);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                }, std::to_string((size_t)To<U>(L)));
                return 0;
            });
            // 这个函数因为 lua 无法 override update 而修改，利用 unschedule 来实现类似效果。key 使用指针值来生成
            SetFieldCClosure(L, "unscheduleUpdate", [](auto L)->int { To<U>(L)->unschedule(std::to_string((size_t)To<U>(L))); return 0; });
            SetFieldCClosure(L, "scheduleOnce", [](auto L)->int {
                // 参数：callback(float)->void, float delay
                To<U>(L)->scheduleOnce([f = To<Func>(L, 2)](float delta) {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(delta);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                }, To<float>(L, 3), std::to_string((size_t)To<U>(L)));
                return 0;
            });
            SetFieldCClosure(L, "schedule", [](auto L)->int {
                // 原型: void schedule(const std::function<void(float)>& callback, float interval, unsigned int repeat, float delay, const std::string &key);
                To<U>(L)->schedule([f = To<Func>(L, 2)](float delta)->void {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f(delta);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                }, To<float>(L, 3), To<uint32_t>(L, 4), To<float>(L, 5), To<std::string>(L, 6));
                return 0;
            });
            SetFieldCClosure(L, "unschedule", [](auto L)->int { To<U>(L)->unschedule(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "unscheduleAllCallbacks", [](auto L)->int { To<U>(L)->unscheduleAllCallbacks(); return 0; });
            SetFieldCClosure(L, "resume", [](auto L)->int { To<U>(L)->resume(); return 0; });
            SetFieldCClosure(L, "pause", [](auto L)->int { To<U>(L)->pause(); return 0; });
            SetFieldCClosure(L, "update", [](auto L)->int { To<U>(L)->update(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "updateTransform", [](auto L)->int { To<U>(L)->updateTransform(); return 0; });
            // todo: get/set xxxxx Transform
            SetFieldCClosure(L, "convertToNodeSpace", [](auto L)->int {
                auto&& r = To<U>(L)->convertToNodeSpace({ To<float>(L, 2), To<float>(L, 3) });
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "convertToNodeSpaceAR", [](auto L)->int {
                auto&& r = To<U>(L)->convertToNodeSpaceAR({ To<float>(L, 2), To<float>(L, 3) });
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "convertToWorldSpace", [](auto L)->int {
                auto&& r = To<U>(L)->convertToWorldSpace({ To<float>(L, 2), To<float>(L, 3) });
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "convertToWorldSpaceAR", [](auto L)->int {
                auto&& r = To<U>(L)->convertToWorldSpaceAR({ To<float>(L, 2), To<float>(L, 3) });
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "convertTouchToNodeSpace", [](auto L)->int {
                auto&& r = To<U>(L)->convertTouchToNodeSpace(To<cocos2d::Touch*>(L, 2));
                return Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "convertTouchToNodeSpaceAR", [](auto L)->int {
                auto&& r = To<U>(L)->convertTouchToNodeSpaceAR(To<cocos2d::Touch*>(L, 2));
                return Push(L, r.width, r.height);
            });
            // todo: get / set / add / remove  Component
            SetFieldCClosure(L, "getOpacity", [](auto L)->int { return Push(L, To<U>(L)->getOpacity()); });
            SetFieldCClosure(L, "getDisplayedOpacity", [](auto L)->int { return Push(L, To<U>(L)->getDisplayedOpacity()); });
            SetFieldCClosure(L, "setOpacity", [](auto L)->int { To<U>(L)->setOpacity(To<uint8_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "updateDisplayedOpacity", [](auto L)->int { To<U>(L)->updateDisplayedOpacity(To<uint8_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "isCascadeOpacityEnabled", [](auto L)->int { return Push(L, To<U>(L)->isCascadeOpacityEnabled()); });
            SetFieldCClosure(L, "setCascadeOpacityEnabled", [](auto L)->int { To<U>(L)->setCascadeOpacityEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "getColor", [](auto L)->int {
                auto&& o = To<U>(L)->getColor(); 
                return Push(L, o.r, o.g, o.b);
            });
            SetFieldCClosure(L, "getDisplayedColor", [](auto L)->int {
                auto&& o = To<U>(L)->getDisplayedColor();
                return Push(L, o.r, o.g, o.b);
            });
            SetFieldCClosure(L, "setColor", [](auto L)->int { To<U>(L)->setColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4) }); return 0; });
            SetFieldCClosure(L, "updateDisplayedColor", [](auto L)->int { To<U>(L)->updateDisplayedColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4) }); return 0; });
            SetFieldCClosure(L, "isCascadeColorEnabled", [](auto L)->int { return Push(L, To<U>(L)->isCascadeColorEnabled()); });
            SetFieldCClosure(L, "setCascadeColorEnabled", [](auto L)->int { To<U>(L)->setCascadeColorEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "setOpacityModifyRGB", [](auto L)->int { To<U>(L)->setOpacityModifyRGB(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isOpacityModifyRGB", [](auto L)->int { return Push(L, To<U>(L)->isOpacityModifyRGB()); });
            SetFieldCClosure(L, "setOnEnterCallback", [](auto L)->int {
                To<U>(L)->setOnEnterCallback([f = To<Func>(L, 2)]{
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f();
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                });
                return 0;
            });
            // todo: getOnEnterCallback
            SetFieldCClosure(L, "setOnExitCallback", [](auto L)->int {
                To<U>(L)->setOnExitCallback([f = To<Func>(L, 2)] {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f();
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                });
                return 0;
            });
            // todo: getOnExitCallback
            SetFieldCClosure(L, "setOnEnterTransitionDidFinishCallback", [](auto L)->int {
                To<U>(L)->setOnEnterTransitionDidFinishCallback([f = To<Func>(L, 2)] {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f();
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                });
                return 0;
            });
            // todo: getOnEnterTransitionDidFinishCallback
            SetFieldCClosure(L, "setOnExitTransitionDidStartCallback", [](auto L)->int {
                To<U>(L)->setOnExitTransitionDidStartCallback([f = To<Func>(L, 2)] {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    if (auto&& r = Try(_luaState, [&] {
#endif
                        f();
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
                });
                return 0;
            });
            // todo: getOnExitTransitionDidStartCallback
            SetFieldCClosure(L, "setCameraMask", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setCameraMask(To<int>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->setCameraMask(To<int>(L, 2), To<bool>(L, 3));
                    return 0;
                }
                default:
                    return luaL_error(L, "%s", "setCameraMask error! need 2 ~ 3 args: self, unsigned short mask, bool applyChildren = true");
                }
            });
            SetFieldCClosure(L, "getCameraMask", [](auto L)->int { return Push(L, To<U>(L)->getCameraMask()); });
            // todo: setProgramState setProgramStateWithRegistry getProgramState
            SetFieldCClosure(L, "updateProgramStateTexture", [](auto L)->int { To<U>(L)->updateProgramStateTexture(To<cocos2d::Texture2D*>(L, 2)); return 0; });
            SetFieldCClosure(L, "resetChild", [](auto L)->int { To<U>(L)->resetChild(To<cocos2d::Node*>(L, 2), To<bool>(L, 3)); return 0; });
            //SetFieldCClosure(L, "setPhysicsBody", [](auto L)->int { To<U>(L)->setPhysicsBody(To<cocos2d::PhysicsBody*>(L, 2)); return 0; });
            //SetFieldCClosure(L, "getPhysicsBody", [](auto L)->int { return Push(L, To<U>(L)->getPhysicsBody()); });
        }
    };

    /*******************************************************************************************/
    // BaseLight : Node
    template<>
    struct MetaFuncs<cocos2d::BaseLight*, void> {
        using U = cocos2d::BaseLight*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // Camera : Node
    template<>
    struct MetaFuncs<cocos2d::Camera*, void> {
        using U = cocos2d::Camera*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // SpriteBatchNode : Node, TextureProtocol
    template<>
    struct MetaFuncs<cocos2d::SpriteBatchNode*, void> {
        using U = cocos2d::SpriteBatchNode*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // Scene : Node
    template<>
    struct MetaFuncs<cocos2d::Scene*, void> {
        using U = cocos2d::Scene*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getCameras", [](auto L)->int { return Push(L, To<U>(L)->getCameras()); });
            SetFieldCClosure(L, "getDefaultCamera", [](auto L)->int { return Push(L, To<U>(L)->getDefaultCamera()); });
            SetFieldCClosure(L, "getLights", [](auto L)->int { return Push(L, To<U>(L)->getLights()); });
        }
    };

    /*******************************************************************************************/
    // Sprite : Node
    template<>
    struct MetaFuncs<cocos2d::Sprite*, void> {
        using U = cocos2d::Sprite*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getBatchNode", [](auto L)->int { return Push(L, To<U>(L)->getBatchNode()); });
            SetFieldCClosure(L, "setBatchNode", [](auto L)->int { To<U>(L)->setBatchNode(To<cocos2d::SpriteBatchNode*>(L, 2)); return 0; });
            SetFieldCClosure(L, "setTexture", [](auto L)->int {
                if (lua_isstring(L, 2)) {
                    To<U>(L)->setTexture(To<std::string>(L, 2));
                }
                else {
                    To<U>(L)->setTexture(To<cocos2d::Texture2D*>(L, 2));
                }
                return 0;
            });
            SetFieldCClosure(L, "getTexture", [](auto L)->int { return Push(L, To<U>(L)->getTexture()); });
            SetFieldCClosure(L, "setTextureRect", [](auto L)->int { 
                switch (lua_gettop(L)) {
                case 5: {
                    To<U>(L)->setTextureRect({ XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) });
                    return 0;
                }
                case 8: {
                    To<U>(L)->setTextureRect({ XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) }
                    , XL::To<bool>(L, 6), { XL::To<float>(L, 7), XL::To<float>(L, 8) });
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "Sprite:setTextureRect error! need 5 / 8 args: Sprite* self, Rect rect{ float x, y, w, h }, bool rotated, Vec2 untrimmedSize{ float w, h }");
                }
                }
            });
            SetFieldCClosure(L, "setVertexRect", [](auto L)->int {
                To<U>(L)->setVertexRect({ XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "setCenterRectNormalized", [](auto L)->int {
                To<U>(L)->setCenterRectNormalized({ XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "getCenterRectNormalized", [](auto L)->int {
                auto&& r = To<U>(L)->getCenterRectNormalized();
                return XL::Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            SetFieldCClosure(L, "setCenterRect", [](auto L)->int {
                To<U>(L)->setCenterRect({ XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "getCenterRect", [](auto L)->int {
                auto&& r = To<U>(L)->getCenterRect();
                return XL::Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            SetFieldCClosure(L, "setSpriteFrame", [](auto L)->int {
                if (lua_isstring(L, 1)) {
                    To<U>(L)->setSpriteFrame(To<std::string>(L, 2));
                }
                else {
                    To<U>(L)->setSpriteFrame(To<cocos2d::SpriteFrame*>(L, 2));
                }
                return 0;
            });
            SetFieldCClosure(L, "getSpriteFrame", [](auto L)->int { return Push(L, To<U>(L)->getSpriteFrame()); });
            SetFieldCClosure(L, "setDisplayFrameWithAnimationName", [](auto L)->int {
                To<U>(L)->setDisplayFrameWithAnimationName( XL::To<std::string>(L, 2), XL::To<uint32_t>(L, 3));
                return 0;
            });
            SetFieldCClosure(L, "isDirty", [](auto L)->int { return Push(L, To<U>(L)->isDirty()); });
            SetFieldCClosure(L, "setDirty", [](auto L)->int { To<U>(L)->setDirty(To<bool>(L, 2)); return 0; });
            // todo: getQuad 
            SetFieldCClosure(L, "isTextureRectRotated", [](auto L)->int { return Push(L, To<U>(L)->isTextureRectRotated()); });
            SetFieldCClosure(L, "getAtlasIndex", [](auto L)->int { return Push(L, To<U>(L)->getAtlasIndex()); });
            SetFieldCClosure(L, "setAtlasIndex", [](auto L)->int { To<U>(L)->setAtlasIndex(XL::To<uint32_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "getTextureRect", [](auto L)->int {
                auto&& r = To<U>(L)->getTextureRect();
                return XL::Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            SetFieldCClosure(L, "getTextureAtlas", [](auto L)->int { To<U>(L)->getTextureAtlas(); return 0; });
            SetFieldCClosure(L, "setTextureAtlas", [](auto L)->int { To<U>(L)->setTextureAtlas(XL::To<cocos2d::TextureAtlas*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getOffsetPosition", [](auto L)->int {
                auto&& r = To<U>(L)->getOffsetPosition();
                return XL::Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "isFlippedX", [](auto L)->int { return Push(L, To<U>(L)->isFlippedX()); });
            SetFieldCClosure(L, "setFlippedX", [](auto L)->int { To<U>(L)->setFlippedX(XL::To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isFlippedY", [](auto L)->int { return Push(L, To<U>(L)->isFlippedY()); });
            SetFieldCClosure(L, "setFlippedY", [](auto L)->int { To<U>(L)->setFlippedY(XL::To<bool>(L, 2)); return 0; });
            // todo: getPolygonInfo setPolygonInfo
            SetFieldCClosure(L, "setStretchEnabled", [](auto L)->int { To<U>(L)->setStretchEnabled(XL::To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isStretchEnabled", [](auto L)->int { return Push(L, To<U>(L)->isStretchEnabled()); });
            SetFieldCClosure(L, "getResourceType", [](auto L)->int { return Push(L, To<U>(L)->getResourceType()); });
            SetFieldCClosure(L, "getResourceName", [](auto L)->int { return Push(L, To<U>(L)->getResourceName()); });
        }
    };

    /*******************************************************************************************/
    // SpriteFrame : Ref, Cloneable
    template<>
    struct MetaFuncs<cocos2d::SpriteFrame*, void> {
        using U = cocos2d::SpriteFrame*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // Texture2D : Ref
    template<>
    struct MetaFuncs<cocos2d::Texture2D*, void> {
        using U = cocos2d::Texture2D*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // TextureAtlas : Ref
    template<>
    struct MetaFuncs<cocos2d::TextureAtlas*, void> {
        using U = cocos2d::TextureAtlas*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            // todo
        }
    };

    /*******************************************************************************************/
    // Label : Node, LabelProtocol, BlendProtocol
    template<>
    struct MetaFuncs<cocos2d::Label*, void> {
        using U = cocos2d::Label*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setTTFConfig", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 1: {
                    To<U>(L)->setTTFConfig({});
                    return 0;
                }
                case 2: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2) });
                    return 0;
                }
                case 3: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3) });
                    return 0;
                }
                case 4: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4) });
                    return 0;
                }
                case 5: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5) });
                    return 0;
                }
                case 6: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6) });
                    return 0;
                }
                case 7: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6), To<int>(L, 7) });
                    return 0;
                }
                case 8: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6), To<int>(L, 7)
                        , To<bool>(L, 8) });
                    return 0;
                }
                case 9: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6), To<int>(L, 7)
                        , To<bool>(L, 8), To<bool>(L, 9) });
                    return 0;
                }
                case 10: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6), To<int>(L, 7)
                        , To<bool>(L, 8), To<bool>(L, 9), To<bool>(L, 10) });
                    return 0;
                }
                case 11: {
                    To<U>(L)->setTTFConfig({ To<std::string_view>(L, 2), To<float>(L, 3), To<cocos2d::GlyphCollection>(L, 4)
                        , To<std::string_view>(L, 5), To<bool>(L, 6), To<int>(L, 7)
                        , To<bool>(L, 8), To<bool>(L, 9), To<bool>(L, 10), To<bool>(L, 11) });
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "setTTFConfig error! need 1 ~ 11 args: self, string filePath = \"\", float size = CC_DEFAULT_FONT_LABEL_SIZE, GlyphCollection glyphCollection = GlyphCollection::DYNAMIC, const char *customGlyphCollection = nullptr, bool useDistanceField = false, int outline = 0, bool useItalics = false, bool useBold = false, bool useUnderline = false, bool useStrikethrough = false");
                }
                }
            });
            SetFieldCClosure(L, "getTTFConfig", [](auto L)->int {
                auto&& o = To<U>(L)->getTTFConfig();
                return Push(L, o.fontFilePath, o.fontSize, o.glyphs, o.customGlyphs, o.distanceFieldEnabled, o.outlineSize, o.italics, o.bold, o.underline, o.strikethrough);
            });
            SetFieldCClosure(L, "setBMFontFilePath", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setBMFontFilePath(To<std::string>(L, 2));
                    return 0;
                }
                case 3: {
                    if (lua_isnumber(L, 3)) {
                        To<U>(L)->setBMFontFilePath(To<std::string>(L, 2), To<float>(L, 3));
                    }
                    else {
                        To<U>(L)->setBMFontFilePath(To<std::string>(L, 2), To<std::string>(L, 3));
                    }
                    return 0;
                }
                case 7: {
                    To<U>(L)->setBMFontFilePath(To<std::string>(L, 2), { XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5), XL::To<float>(L, 6) }, XL::To<bool>(L, 7));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "setBMFontFilePath error! need 2, 3, 7 args: self, string bmfontFilePath, float fontSize / string subTextureKey / Rect {float x, y, w, h}, bool imageRotated");
                }
                }
            });
            SetFieldCClosure(L, "getBMFontFilePath", [](auto L)->int { return Push(L, To<U>(L)->getBMFontFilePath()); });
            SetFieldCClosure(L, "setCharMap", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setCharMap(To<std::string>(L, 2));
                    return 0;
                }
                case 5: {
                    if (lua_isstring(L, 2)) {
                        To<U>(L)->setCharMap(To<std::string>(L, 2), To<int>(L, 3), To<int>(L, 3), To<int>(L, 3));
                    }
                    else {
                        To<U>(L)->setCharMap(To<cocos2d::Texture2D*>(L, 2), To<int>(L, 3), To<int>(L, 3), To<int>(L, 3));
                    }
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "setCharMap error! need 2, 5 args: self, string plistFile | string charMapFile / Texture2D* texture, int itemWidth, int itemHeight, int startCharMap");
                }
                }
            });
            SetFieldCClosure(L, "setSystemFontName", [](auto L)->int { To<U>(L)->setSystemFontName(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "getSystemFontName", [](auto L)->int { return Push(L, To<U>(L)->getSystemFontName()); });
            SetFieldCClosure(L, "setSystemFontSize", [](auto L)->int { To<U>(L)->setSystemFontSize(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getSystemFontSize", [](auto L)->int { return Push(L, To<U>(L)->getSystemFontSize()); });
            SetFieldCClosure(L, "requestSystemFontRefresh", [](auto L)->int { To<U>(L)->requestSystemFontRefresh(); return 0; });
            SetFieldCClosure(L, "setString", [](auto L)->int { To<U>(L)->setString(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "getString", [](auto L)->int { return Push(L, To<U>(L)->getString()); });
            SetFieldCClosure(L, "getStringNumLines", [](auto L)->int { return Push(L, To<U>(L)->getStringNumLines()); });
            SetFieldCClosure(L, "getStringLength", [](auto L)->int { return Push(L, To<U>(L)->getStringLength()); });
            SetFieldCClosure(L, "setTextColor", [](auto L)->int {
                To<U>(L)->setTextColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "getTextColor", [](auto L)->int {
                auto&& o = To<U>(L)->getTextColor();
                return Push(L, o.r, o.g, o.b, o.a);
            });
            SetFieldCClosure(L, "enableShadow", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 1: {
                    To<U>(L)->enableShadow();
                    return 0;
                }
                case 5: {
                    To<U>(L)->enableShadow({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) });
                    return 0;
                }
                case 7: {
                    To<U>(L)->enableShadow({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) }, { To<float>(L, 6), To<float>(L, 7) });
                    return 0;
                }
                case 8: {
                    To<U>(L)->enableShadow({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) }, { To<float>(L, 6), To<float>(L, 7) }, To<int>(L, 8));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "enableShadow error! need 1, 5 ~ 8 args: self, GLubyte r = 0, g = 0, b = 0, a = 0, float offsetW = 2, offsetH = -2, int blurRadius = 0");
                }
                }
            });
            SetFieldCClosure(L, "enableOutline", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 5: {
                    To<U>(L)->enableOutline({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) });
                    return 0;
                }
                case 6: {
                    To<U>(L)->enableOutline({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) }, To<int>(L, 6));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "enableOutline error! need 5 ~ 6 args: self, GLubyte r = 0, g = 0, b = 0, a = 0, int outlineSize = -1");
                }
                }
            });
            SetFieldCClosure(L, "enableGlow", [](auto L)->int {
                To<U>(L)->enableGlow({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "enableItalics", [](auto L)->int { To<U>(L)->enableItalics(); return 0; });
            SetFieldCClosure(L, "enableBold", [](auto L)->int { To<U>(L)->enableBold(); return 0; });
            SetFieldCClosure(L, "enableUnderline", [](auto L)->int { To<U>(L)->enableUnderline(); return 0; });
            SetFieldCClosure(L, "enableStrikethrough", [](auto L)->int { To<U>(L)->enableStrikethrough(); return 0; });
            SetFieldCClosure(L, "disableEffect", [](auto L)->int { 
                if (lua_gettop(L)) {
                    To<U>(L)->disableEffect(To<cocos2d::LabelEffect>(L, 2));
                }
                else {
                    To<U>(L)->disableEffect();
                }
                return 0;
            });
            SetFieldCClosure(L, "isShadowEnabled", [](auto L)->int { return Push(L, To<U>(L)->isShadowEnabled()); });
            SetFieldCClosure(L, "getShadowOffset", [](auto L)->int { 
                auto&& o = To<U>(L)->getShadowOffset();
                return Push(L, o.x, o.y);
            });
            SetFieldCClosure(L, "getShadowBlurRadius", [](auto L)->int { return Push(L, To<U>(L)->getShadowBlurRadius()); });
            SetFieldCClosure(L, "getShadowColor", [](auto L)->int { 
                auto&& o = To<U>(L)->getShadowColor();
                return Push(L, o.r, o.g, o.b, o.a); // Color4F
            });
            SetFieldCClosure(L, "getOutlineSize", [](auto L)->int { return Push(L, To<U>(L)->getOutlineSize()); });
            SetFieldCClosure(L, "getLabelEffectType", [](auto L)->int { return Push(L, To<U>(L)->getLabelEffectType()); });
            SetFieldCClosure(L, "getEffectColor", [](auto L)->int { 
                auto&& o = To<U>(L)->getEffectColor();
                return Push(L, o.r, o.g, o.b, o.a); // Color4F
            });
            SetFieldCClosure(L, "setAlignment", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setAlignment(To<cocos2d::TextHAlignment>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->setAlignment(To<cocos2d::TextHAlignment>(L, 2), To<cocos2d::TextVAlignment>(L, 3));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "setAlignment error! need 2 ~ 3 args: self, TextHAlignment hAlignment, TextVAlignment vAlignment");
                }
                }
            });
            SetFieldCClosure(L, "getTextAlignment", [](auto L)->int { return Push(L, To<U>(L)->getTextAlignment()); });
            SetFieldCClosure(L, "setHorizontalAlignment", [](auto L)->int { To<U>(L)->setHorizontalAlignment(To<cocos2d::TextHAlignment>(L, 2)); return 0; });
            SetFieldCClosure(L, "getHorizontalAlignment", [](auto L)->int { return Push(L, To<U>(L)->getHorizontalAlignment()); });
            SetFieldCClosure(L, "setVerticalAlignment", [](auto L)->int { To<U>(L)->setVerticalAlignment(To<cocos2d::TextVAlignment>(L, 2)); return 0; });
            SetFieldCClosure(L, "getVerticalAlignment", [](auto L)->int { return Push(L, To<U>(L)->getVerticalAlignment()); });
            SetFieldCClosure(L, "setLineBreakWithoutSpace", [](auto L)->int { To<U>(L)->setLineBreakWithoutSpace(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "setMaxLineWidth", [](auto L)->int { To<U>(L)->setMaxLineWidth(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getMaxLineWidth", [](auto L)->int { return Push(L, To<U>(L)->getMaxLineWidth()); });
            SetFieldCClosure(L, "setBMFontSize", [](auto L)->int { To<U>(L)->setBMFontSize(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getBMFontSize", [](auto L)->int { return Push(L, To<U>(L)->getBMFontSize()); });
            SetFieldCClosure(L, "enableWrap", [](auto L)->int { To<U>(L)->enableWrap(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isWrapEnabled", [](auto L)->int { return Push(L, To<U>(L)->isWrapEnabled()); });
            SetFieldCClosure(L, "setOverflow", [](auto L)->int { To<U>(L)->setOverflow(To<cocos2d::Label::Overflow>(L, 2)); return 0; });
            SetFieldCClosure(L, "getOverflow", [](auto L)->int { return Push(L, To<U>(L)->getOverflow()); });
            SetFieldCClosure(L, "setWidth", [](auto L)->int { To<U>(L)->setWidth(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getWidth", [](auto L)->int { return Push(L, To<U>(L)->getWidth()); });
            SetFieldCClosure(L, "setHeight", [](auto L)->int { To<U>(L)->setHeight(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getHeight", [](auto L)->int { return Push(L, To<U>(L)->getHeight()); });
            SetFieldCClosure(L, "setDimensions", [](auto L)->int { To<U>(L)->setDimensions(To<float>(L, 2), To<float>(L, 3)); return 0; });
            SetFieldCClosure(L, "getDimensions", [](auto L)->int { 
                auto&& o = To<U>(L)->getDimensions();
                return Push(L, o.width, o.height);
            });
            SetFieldCClosure(L, "updateContent", [](auto L)->int { To<U>(L)->updateContent(); return 0; });
            SetFieldCClosure(L, "getLetter", [](auto L)->int { return Push(L, To<U>(L)->getLetter(To<int>(L, 2))); });
            SetFieldCClosure(L, "setClipMarginEnabled", [](auto L)->int { To<U>(L)->setClipMarginEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isClipMarginEnabled", [](auto L)->int { return Push(L, To<U>(L)->isClipMarginEnabled()); });
            SetFieldCClosure(L, "setLineHeight", [](auto L)->int { To<U>(L)->setLineHeight(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getLineHeight", [](auto L)->int { return Push(L, To<U>(L)->getLineHeight()); });
            SetFieldCClosure(L, "setLineSpacing", [](auto L)->int { To<U>(L)->setLineSpacing(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getLineSpacing", [](auto L)->int { return Push(L, To<U>(L)->getLineSpacing()); });
            SetFieldCClosure(L, "getLabelType", [](auto L)->int { return Push(L, To<U>(L)->getLabelType()); });
            SetFieldCClosure(L, "getRenderingFontSize", [](auto L)->int { return Push(L, To<U>(L)->getRenderingFontSize()); });
            SetFieldCClosure(L, "setAdditionalKerning", [](auto L)->int { To<U>(L)->setAdditionalKerning(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getAdditionalKerning", [](auto L)->int { return Push(L, To<U>(L)->getAdditionalKerning()); });
            SetFieldCClosure(L, "getFontAtlas", [](auto L)->int { return Push(L, To<U>(L)->getFontAtlas()); });

            // todo
        }
    };

    /*******************************************************************************************/
    // ScrollView : 简化为 Node ( 实际为 Layer, ActionTweenDelegate )
    template<>
    struct MetaFuncs<cocos2d::extension::ScrollView*, void> {
        using U = cocos2d::extension::ScrollView*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "setContentOffset", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 3: {
                    To<U>(L)->setContentOffset({ To<float>(L, 2) , To<float>(L, 3) }); 
                    return 0;
                }
                case 4: {
                    To<U>(L)->setContentOffset({ To<float>(L, 2) , To<float>(L, 3) }, To<bool>(L, 4));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "ScrollView:setContentOffset error! need 3 ~ 4 args: self, Vec2 offset{ float x, y }, bool animated = false");
                }
                }
            });
            SetFieldCClosure(L, "getContentOffset", [](auto L)->int {
                auto&& r = To<U>(L)->getContentOffset();
                return XL::Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "setContentOffsetInDuration", [](auto L)->int {
                To<U>(L)->setContentOffsetInDuration({ To<float>(L, 2) , To<float>(L, 3) }, To<float>(L, 4));
                return 0;
            });
            SetFieldCClosure(L, "stopAnimatedContentOffset", [](auto L)->int { To<U>(L)->stopAnimatedContentOffset(); return 0; });
            SetFieldCClosure(L, "setZoomScale", [](auto L)->int {
                switch (lua_gettop(L)) {
                case 2: {
                    To<U>(L)->setZoomScale(To<float>(L, 2));
                    return 0;
                }
                case 3: {
                    To<U>(L)->setZoomScale(To<float>(L, 2), To<bool>(L, 3));
                    return 0;
                }
                default: {
                    return luaL_error(L, "%s", "ScrollView:setZoomScale error! need 2 ~ 3 args: self, float s, bool animated");
                }
                }
            });
            SetFieldCClosure(L, "getZoomScale", [](auto L)->int { return Push(L, To<U>(L)->getZoomScale()); });
            SetFieldCClosure(L, "setZoomScaleInDuration", [](auto L)->int { To<U>(L)->setZoomScaleInDuration(To<float>(L, 2), To<float>(L, 3)); return 0; });
            SetFieldCClosure(L, "setMinScale", [](auto L)->int { To<U>(L)->setMinScale(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "setMaxScale", [](auto L)->int { To<U>(L)->setMaxScale(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "minContainerOffset", [](auto L)->int {
                auto&& r = To<U>(L)->minContainerOffset();
                return XL::Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "maxContainerOffset", [](auto L)->int {
                auto&& r = To<U>(L)->maxContainerOffset();
                return XL::Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "isNodeVisible", [](auto L)->int { return Push(L, To<U>(L)->isNodeVisible(To<cocos2d::Node*>(L, 2))); });
            SetFieldCClosure(L, "pause", [](auto L)->int { To<U>(L)->pause(To<cocos2d::Ref*>(L, 2)); return 0; });
            SetFieldCClosure(L, "resume", [](auto L)->int { To<U>(L)->resume(To<cocos2d::Ref*>(L, 2)); return 0; });
            SetFieldCClosure(L, "setTouchEnabled", [](auto L)->int { To<U>(L)->setTouchEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isTouchEnabled", [](auto L)->int { return Push(L, To<U>(L)->isTouchEnabled()); });
            SetFieldCClosure(L, "setSwallowTouches", [](auto L)->int { To<U>(L)->setSwallowTouches(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isDragging", [](auto L)->int { return Push(L, To<U>(L)->isDragging()); });
            SetFieldCClosure(L, "isTouchMoved", [](auto L)->int { return Push(L, To<U>(L)->isTouchMoved()); });
            SetFieldCClosure(L, "isBounceable", [](auto L)->int { return Push(L, To<U>(L)->isBounceable()); });
            SetFieldCClosure(L, "setBounceable", [](auto L)->int { To<U>(L)->setBounceable(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "getViewSize", [](auto L)->int {
                auto&& r = To<U>(L)->getViewSize();
                return XL::Push(L, r.width, r.height);
            });
            SetFieldCClosure(L, "setViewSize", [](auto L)->int { To<U>(L)->setViewSize({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getContainer", [](auto L)->int { return Push(L, To<U>(L)->getContainer()); });
            SetFieldCClosure(L, "setContainer", [](auto L)->int { To<U>(L)->setContainer(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "getDirection", [](auto L)->int { return Push(L, To<U>(L)->getDirection()); });
            SetFieldCClosure(L, "setDirection", [](auto L)->int { To<U>(L)->setDirection(To<cocos2d::extension::ScrollView::Direction>(L, 2)); return 0; });
            // getDelegate, setDelegate
            SetFieldCClosure(L, "updateInset", [](auto L)->int { To<U>(L)->updateInset(); return 0; });
            SetFieldCClosure(L, "isClippingToBounds", [](auto L)->int { return Push(L, To<U>(L)->isClippingToBounds()); });
            SetFieldCClosure(L, "setClippingToBounds", [](auto L)->int { To<U>(L)->setClippingToBounds(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "hasVisibleParents", [](auto L)->int { return Push(L, To<U>(L)->hasVisibleParents()); });
            // onTouchBegan onTouchMoved onTouchEnded onTouchCancelled
        }
    };


    /*******************************************************************************************/
    // ClippingNode : Node
    template<>
    struct MetaFuncs<cocos2d::ClippingNode*, void> {
        using U = cocos2d::ClippingNode*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getStencil", [](auto L)->int { return Push(L, To<U>(L)->getStencil()); });
            SetFieldCClosure(L, "setStencil", [](auto L)->int { To<U>(L)->setStencil(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "hasContent", [](auto L)->int { return Push(L, To<U>(L)->hasContent()); });
            SetFieldCClosure(L, "getAlphaThreshold", [](auto L)->int { return Push(L, To<U>(L)->getAlphaThreshold()); });
            SetFieldCClosure(L, "setAlphaThreshold", [](auto L)->int { To<U>(L)->setAlphaThreshold(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "isInverted", [](auto L)->int { return Push(L, To<U>(L)->isInverted()); });
            SetFieldCClosure(L, "setInverted", [](auto L)->int { To<U>(L)->setInverted(To<bool>(L, 2)); return 0; });
        }
    };

    /*******************************************************************************************/
    // ClippingRectangleNode : Node
    template<>
    struct MetaFuncs<cocos2d::ClippingRectangleNode*, void> {
        using U = cocos2d::ClippingRectangleNode*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getClippingRegion", [](auto L)->int {
                auto&& r = To<U>(L)->getClippingRegion();
                return Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            SetFieldCClosure(L, "setClippingRegion", [](auto L)->int {
                To<U>(L)->setClippingRegion({ To<float>(L, 2), To<float>(L, 3), To<float>(L, 4), To<float>(L, 5) });
                return 0; 
            });
            SetFieldCClosure(L, "isClippingEnabled", [](auto L)->int { return Push(L, To<U>(L)->isClippingEnabled()); });
            SetFieldCClosure(L, "setClippingEnabled", [](auto L)->int { To<U>(L)->setClippingEnabled(To<bool>(L, 2)); return 0; });
        }
    };

    /*******************************************************************************************/
    // DrawNode : Node
    template<>
    struct MetaFuncs<cocos2d::DrawNode*, void> {
        using U = cocos2d::DrawNode*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Node*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "drawPoint", [](auto L)->int {
                To<U>(L)->drawPoint({ To<float>(L, 2), To<float>(L, 3) }, To<float>(L, 4)
                    , { To<uint8_t>(L, 5), To<uint8_t>(L, 6), To<uint8_t>(L, 7), To<uint8_t>(L, 8) });
                return 0;
            });
            SetFieldCClosure(L, "drawPoints", [](auto L)->int {
                auto top = lua_gettop(L);
                if (top < 8 || ((top - 8) & 1) == 1) return luaL_error(L, "%s", "DrawNode:drawPoints error! need 8+ args: self, float pointSize, Color4B color{ byte r,g,b,a }, Vec2{ float x, y }[] points...");
                auto c = (top - 6) << 1;
                auto p = _bh.Get<cocos2d::Vec2>(c);
                auto o = p;
                for (auto i = 7; i < top; i += 2, ++o) {
                    (*o).x = To<float>(L, i);
                    (*o).y = To<float>(L, i + 1);
                }
                To<U>(L)->drawPoints(p, c, To<float>(L, 2), { To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5), To<uint8_t>(L, 6) });
                return 0;
            });
            SetFieldCClosure(L, "drawLine", [](auto L)->int {
                To<U>(L)->drawLine({ To<float>(L, 2), To<float>(L, 3) }, { To<float>(L, 4), To<float>(L, 5) }
                    , { To<uint8_t>(L, 6), To<uint8_t>(L, 7), To<uint8_t>(L, 8), To<uint8_t>(L, 9)});
                return 0;
            });
            SetFieldCClosure(L, "drawRect", [](auto L)->int {
                To<U>(L)->drawRect({ To<float>(L, 2), To<float>(L, 3) }, { To<float>(L, 4), To<float>(L, 5) }
                    , { To<uint8_t>(L, 6), To<uint8_t>(L, 7), To<uint8_t>(L, 8), To<uint8_t>(L, 9)});
                return 0;
            });
            SetFieldCClosure(L, "drawPoly", [](auto L)->int {
                auto top = lua_gettop(L);
                if (top < 8 || ((top - 8) & 1) == 1) return luaL_error(L, "%s", "DrawNode:drawPoly error! need 8+ args: self, bool closePolygon, Color4B color{ byte r,g,b,a }, Vec2{ float x, y }[] points...");
                auto c = (top - 6) << 1;
                auto p = _bh.Get<cocos2d::Vec2>(c);
                auto o = p;
                for (auto i = 7; i < top; i += 2, ++o) {
                    (*o).x = To<float>(L, i);
                    (*o).y = To<float>(L, i + 1);
                }
                To<U>(L)->drawPoints(p, c, To<bool>(L, 2), { To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5), To<uint8_t>(L, 6) });
                return 0;
            });
            SetFieldCClosure(L, "drawCircle", [](auto L)->int {
                To<U>(L)->drawCircle({ To<float>(L, 2), To<float>(L, 3) }
                    , To<float>(L, 4), To<float>(L, 5), To<uint32_t>(L, 6), To<bool>(L, 7)
                    , To<float>(L, 8), To<float>(L, 9)
                    , { To<uint8_t>(L, 10), To<uint8_t>(L, 11), To<uint8_t>(L, 12), To<uint8_t>(L, 13)});
                return 0;
            });
            SetFieldCClosure(L, "drawQuadBezier", [](auto L)->int {
                To<U>(L)->drawQuadBezier({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , { To<float>(L, 6), To<float>(L, 7) }
                    , To<uint32_t>(L, 8)
                    , { To<uint8_t>(L, 9), To<uint8_t>(L, 10), To<uint8_t>(L, 11), To<uint8_t>(L, 12)});
                return 0;
            });
            SetFieldCClosure(L, "drawCubicBezier", [](auto L)->int {
                To<U>(L)->drawCubicBezier({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , { To<float>(L, 6), To<float>(L, 7) }
                    , { To<float>(L, 8), To<float>(L, 9) }
                    , To<uint32_t>(L, 10)
                    , { To<uint8_t>(L, 11), To<uint8_t>(L, 12), To<uint8_t>(L, 13), To<uint8_t>(L, 14)});
                return 0;
            });
            //drawCardinalSpline drawCatmullRom
            SetFieldCClosure(L, "drawDot", [](auto L)->int {
                To<U>(L)->drawDot({ To<float>(L, 2), To<float>(L, 3) }
                    , To<float>(L, 4)
                    , { To<uint8_t>(L, 5), To<uint8_t>(L, 6), To<uint8_t>(L, 7), To<uint8_t>(L, 8)});
                return 0;
            });
            SetFieldCClosure(L, "drawRect", [](auto L)->int {
                To<U>(L)->drawRect({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , { To<float>(L, 6), To<float>(L, 7) }
                    , { To<float>(L, 8), To<float>(L, 9) }
                    , { To<uint8_t>(L, 10), To<uint8_t>(L, 11), To<uint8_t>(L, 12), To<uint8_t>(L, 13)});
                return 0;
            });
            SetFieldCClosure(L, "drawSolidRect", [](auto L)->int {
                To<U>(L)->drawSolidRect({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , { To<uint8_t>(L, 6), To<uint8_t>(L, 7), To<uint8_t>(L, 8), To<uint8_t>(L, 9) });
                return 0;
            });
            SetFieldCClosure(L, "drawSolidPoly", [](auto L)->int {
                auto top = lua_gettop(L);
                if (top < 7 || ((top - 7) & 1) == 1) return luaL_error(L, "%s", "DrawNode:drawSolidPoly error! need 7+ args: self, Color4B color{ byte r,g,b,a }, Vec2{ float x, y }[] points...");
                auto c = (top - 5) << 1;
                auto p = _bh.Get<cocos2d::Vec2>(c);
                auto o = p;
                for (auto i = 6; i < top; i += 2, ++o) {
                    (*o).x = To<float>(L, i);
                    (*o).y = To<float>(L, i + 1);
                }
                To<U>(L)->drawSolidPoly(p, c, { To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4), To<uint8_t>(L, 5) });
                return 0;
            });
            SetFieldCClosure(L, "drawSolidCircle", [](auto L)->int {
                To<U>(L)->drawSolidCircle({ To<float>(L, 2), To<float>(L, 3) }
                    , To<float>(L, 4), To<float>(L, 5)
                    , To<uint32_t>(L, 6)
                    , To<float>(L, 7), To<float>(L, 8)
                    , { To<uint8_t>(L, 9), To<uint8_t>(L, 10), To<uint8_t>(L, 11), To<uint8_t>(L, 12) }
                    , To<float>(L, 13)
                    , { To<uint8_t>(L, 14), To<uint8_t>(L, 15), To<uint8_t>(L, 16), To<uint8_t>(L, 17) });
                return 0;
            });
            SetFieldCClosure(L, "drawSegment", [](auto L)->int {
                To<U>(L)->drawSegment({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , To<float>(L, 6)
                    , { To<uint8_t>(L, 7), To<uint8_t>(L, 8), To<uint8_t>(L, 9), To<uint8_t>(L, 10) });
                return 0;
            });
            // todo: drawPolygon
            SetFieldCClosure(L, "drawTriangle", [](auto L)->int {
                To<U>(L)->drawTriangle({ To<float>(L, 2), To<float>(L, 3) }
                    , { To<float>(L, 4), To<float>(L, 5) }
                    , { To<float>(L, 6), To<float>(L, 7) }
                    , { To<uint8_t>(L, 8), To<uint8_t>(L, 9), To<uint8_t>(L, 10), To<uint8_t>(L, 11) });
                return 0;
            });
        }
    };

    // todo: more Node ?

    /*******************************************************************************************/
    // push + to
    // 适配 cocos2d::Ref 及其派生类的裸指针. 体现为 userdata + meta ( 指针本身 copy )
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>>&& std::is_base_of_v<cocos2d::Ref, std::remove_pointer_t<std::decay_t<T>>>>> {
        using U = std::decay_t<T>;
        static int Push(lua_State* const& L, T&& in) {
            return PushUserdata<U>(L, in);
        }
        static void To(lua_State* const& L, int const& idx, T& out) {
#if XX_LUA_TO_ENABLE_TYPE_CHECK
            EnsureType<U>(L, idx);  // 在 Release 也生效
#endif
            out = *(U*)lua_touserdata(L, idx);
        }
    };
}

void luaBinds(AppDelegate* ad) {
    // 创建环境
    _appDelegate = ad;
    _animationCache = cocos2d::AnimationCache::getInstance();
    _spriteFrameCache = cocos2d::SpriteFrameCache::getInstance();
    _fileUtils = cocos2d::FileUtils::getInstance();
    _director = cocos2d::Director::getInstance();
    _actionManager = _director->getActionManager();
    _scheduler = _director->getScheduler();
    _eventDispatcher = _director->getEventDispatcher();
    auto L = _luaState = luaL_newstate();
    luaL_openlibs(_luaState);

    /***********************************************************************************************/
    // Environment

    // 翻写 cocos 的 get_string_for_print 函数
    static auto get_string_for_print = [](lua_State* L, std::string& out)->int {
        int n = lua_gettop(L);  /* number of arguments */
        int i;

        lua_getglobal(L, "tostring");
        for (i = 1; i <= n; i++) {
            const char* s;
            lua_pushvalue(L, -1);  /* function to be called */
            lua_pushvalue(L, i);   /* value to print */
            lua_call(L, 1, 1);
            size_t sz;
            s = lua_tolstring(L, -1, &sz);  /* get result */
            if (s == NULL)
                return luaL_error(L, "%s", "'tostring' must return a string to 'print'");
            if (i > 1) out.append("\t");
            out.append(s, sz);
            lua_pop(L, 1);  /* pop result */
        }
        return 0;
    };
    
    // 翻写 cocos 的 lua_print
    XL::SetGlobalCClosure(L, "print", [](auto L) -> int {
        _string.clear();
        get_string_for_print(L, _string);
        CCLOG("[LUA-print] %s", _string.c_str());
        return 0;
    });

    // 翻写 cocos 的 lua_release_print
    XL::SetGlobalCClosure(L, "release_print", [](auto L) -> int {
        _string.clear();
        get_string_for_print(L, _string);
        cocos2d::log("[LUA-print] %s", _string.c_str());
        return 0;
    });

    // 翻写 cocos 的 lua_version
    XL::SetGlobalCClosure(L, "lua_version", [](auto L) -> int {
        return XL::Push(L, LUA_VERSION_NUM);
    });

    // 翻写 cocos 的 cocos2dx_lua_loader 函数并加载
    static auto cocos2dx_lua_loader = [](lua_State* L)->int {
        auto relativePath = XL::To<std::string>(L, 1);

        //  convert any '.' to '/'
        size_t pos = relativePath.find_first_of('.');
        while (pos != std::string::npos) {
            relativePath[pos] = '/';
            pos = relativePath.find_first_of('.');
        }

        // search file in package.path
        cocos2d::Data chunk;
        std::string resolvedPath;

        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        auto searchpath = XL::To<std::string_view>(L, -1);
        lua_pop(L, 1);
        size_t begin = 0;
        size_t next = searchpath.find_first_of(';', 0);

        do {
            if (next == std::string::npos) {
                next = searchpath.length();
            }
            auto prefix = searchpath.substr(begin, next - begin);
            if (prefix[0] == '.' && prefix[1] == '/') {
                prefix = prefix.substr(2);
            }

            // reserve enough for file path to avoid memory realloc when replace ? to strPath
            resolvedPath.reserve(prefix.length() + relativePath.length());

            resolvedPath.assign(prefix.data(), prefix.length());
            pos = resolvedPath.find_last_of('?');
            assert(pos != std::string::npos); // package search path should have '?'
            if (pos != std::string::npos) {
                resolvedPath.replace(pos, 1, relativePath);
            }

            if (_fileUtils->isFileExist(resolvedPath)) {
                chunk = _fileUtils->getDataFromFile(resolvedPath);
                break;
            }

            begin = next + 1;
            next = searchpath.find_first_of(';', begin);
        } while (begin < searchpath.length());

        if (chunk.getSize() > 0) {
            resolvedPath.insert(resolvedPath.begin(), '@');  // lua standard, add file chunck mark '@'
            auto chunkName = resolvedPath.c_str();
            auto chunkBuf = (const char*)chunk.getBytes();
            auto chunkSize = (int)chunk.getSize();
            // skipBOM
            if (chunkSize >= 3 
                && (uint8_t)chunkBuf[0] == 0xEF
                && (uint8_t)chunkBuf[1] == 0xBB
                && (uint8_t)chunkBuf[2] == 0xBF) {
                chunkBuf += 3;
                chunkSize -= 3;
            }
            if (int r = luaL_loadbuffer(L, chunkBuf, chunkSize, chunkName)) {   // -0 +1
                switch (r) {
                case LUA_ERRSYNTAX:
                    CCLOG("[LUA ERROR] load \"%s\", error: syntax error during pre-compilation.", chunkName);
                    break;

                case LUA_ERRMEM:
                    CCLOG("[LUA ERROR] load \"%s\", error: memory allocation error.", chunkName);
                    break;

                case LUA_ERRFILE:
                    CCLOG("[LUA ERROR] load \"%s\", error: cannot open/read file.", chunkName);
                    break;

                default:
                    CCLOG("[LUA ERROR] load \"%s\", error: unknown.", chunkName);
                }
            }
        }
        else {
            CCLOG("can not get file data of %s", resolvedPath.c_str());
            return 0;
        }

        return 1;
    };

    {
#if LUA_VERSION_NUM >= 504 || (LUA_VERSION_NUM >= 502 && !defined(LUA_COMPAT_LOADERS))
        const char* realname = "searchers";
#else
        const char* realname = "loaders";
#endif
        // stack content after the invoking of the function
        // get loader table
        lua_getglobal(L, "package");                                /* L: package */
        lua_getfield(L, -1, realname);                              /* L: package, loaders */

        // insert loader into index 2
        lua_pushcfunction(L, cocos2dx_lua_loader);                  /* L: package, loaders, func */
        for (int i = (int)(lua_rawlen(L, -2) + 1); i > 2; --i) {
            lua_rawgeti(L, -2, i - 1);                              /* L: package, loaders, func, function */
            // we call lua_rawgeti, so the loader table now is at -3
            lua_rawseti(L, -3, i);                                  /* L: package, loaders, func */
        }
        lua_rawseti(L, -2, 2);                                      /* L: package, loaders */

        // set loaders into package
        lua_setfield(L, -2, realname);                              /* L: package */

        lua_pop(L, 1);
    }
    XL::AssertTop(L, 0);

    /***********************************************************************************************/
    // Application

    // 注册全局帧回调函数( 利用 _scheduler 实现 )
    XL::SetGlobalCClosure(L, "cc_frameUpdate", [](auto L) -> int {
        To(L, 1, _globalUpdate);
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_enterBackground", [](auto L) -> int {
        To(L, 1, _enterBackground);
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_enterForeground", [](auto L) -> int {
        To(L, 1, _enterForeground);
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setAnimationInterval", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->setAnimationInterval(XL::To<float>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getCurrentLanguage", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->getCurrentLanguage();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getCurrentLanguageCode", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->getCurrentLanguageCode();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getTargetPlatform", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->getTargetPlatform();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getVersion", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->getVersion();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_openURL", [](auto L) -> int {
        ((cocos2d::Application*)_appDelegate)->openURL(XL::To<std::string>(L, 1));
        return 0;
    });


    /***********************************************************************************************/
    // Director

    XL::SetGlobalCClosure(L, "cc_getRunningScene", [](auto L) -> int {
        return XL::Push(L, _director->getRunningScene());
    });

    XL::SetGlobalCClosure(L, "cc_getAnimationInterval", [](auto L) -> int {
        return XL::Push(L, _director->getAnimationInterval());
    });

    XL::SetGlobalCClosure(L, "cc_setAnimationInterval", [](auto L) -> int {
        _director->setAnimationInterval(XL::To<float>(L));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_isDisplayStats", [](auto L) -> int {
        return XL::Push(L, _director->isDisplayStats());
    });

    XL::SetGlobalCClosure(L, "cc_setDisplayStats", [](auto L) -> int {
        _director->setDisplayStats(XL::To<bool>(L));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getSecondsPerFrame", [](auto L) -> int {
        return XL::Push(L, _director->getSecondsPerFrame());
    });

    XL::SetGlobalCClosure(L, "cc_isPaused", [](auto L) -> int {
        return XL::Push(L, _director->isPaused());
    });

    XL::SetGlobalCClosure(L, "cc_getTotalFrames", [](auto L) -> int {
        return XL::Push(L, _director->getTotalFrames());
    });

    XL::SetGlobalCClosure(L, "cc_getProjection", [](auto L) -> int {
        return XL::Push(L, _director->getProjection());
    });

    XL::SetGlobalCClosure(L, "cc_setProjection", [](auto L) -> int {
        _director->setProjection(XL::To<cocos2d::Director::Projection>(L));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getNotificationNode", [](auto L) -> int {
        return XL::Push(L, _director->getNotificationNode());
    });

    XL::SetGlobalCClosure(L, "cc_setNotificationNode", [](auto L) -> int {
        _director->setNotificationNode(XL::To<cocos2d::Node*>(L));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getWinSize", [](auto L) -> int {
        auto&& r = _director->getWinSize();
        return XL::Push(L, r.width, r.height);
    });

    XL::SetGlobalCClosure(L, "cc_getWinSizeInPixels", [](auto L) -> int {
        auto&& r = _director->getWinSizeInPixels();
        return XL::Push(L, r.width, r.height);
    });

    XL::SetGlobalCClosure(L, "cc_getVisibleSize", [](auto L) -> int {
        auto&& r = _director->getVisibleSize();
        return XL::Push(L, r.width, r.height);
    });

    XL::SetGlobalCClosure(L, "cc_getVisibleOrigin", [](auto L) -> int {
        auto&& r = _director->getVisibleOrigin();
        return XL::Push(L, r.x, r.y);
    });

    XL::SetGlobalCClosure(L, "cc_getSafeAreaRect", [](auto L) -> int {
        auto&& r = _director->getSafeAreaRect();
        return XL::Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
    });

    XL::SetGlobalCClosure(L, "cc_convertToGL", [](auto L) -> int {
        auto&& r = _director->convertToGL({ XL::To<float>(L, 1), XL::To<float>(L, 2) });
        return XL::Push(L, r.x, r.y);
    });

    XL::SetGlobalCClosure(L, "cc_convertToUI", [](auto L) -> int {
        auto&& r = _director->convertToUI({ XL::To<float>(L, 1), XL::To<float>(L, 2) });
        return XL::Push(L, r.x, r.y);
    });

    XL::SetGlobalCClosure(L, "cc_getZEye", [](auto L) -> int {
        return XL::Push(L, _director->getZEye());
    });

    XL::SetGlobalCClosure(L, "cc_runWithScene", [](auto L) -> int {
        _director->runWithScene(XL::To<cocos2d::Scene*>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_pushScene", [](auto L) -> int {
        _director->pushScene(XL::To<cocos2d::Scene*>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_popScene", [](auto L) -> int {
        _director->popScene();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_popToRootScene", [](auto L) -> int {
        _director->popToRootScene();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_popToSceneStackLevel", [](auto L) -> int {
        _director->popToSceneStackLevel(XL::To<int>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_replaceScene", [](auto L) -> int {
        _director->replaceScene(XL::To<cocos2d::Scene*>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_end", [](auto L) -> int {
        _director->end();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_pause", [](auto L) -> int {
        _director->pause();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_resume", [](auto L) -> int {
        _director->resume();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_restart", [](auto L) -> int {
        _director->restart();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_stopAnimation", [](auto L) -> int {
        _director->stopAnimation();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_startAnimation", [](auto L) -> int {
        _director->startAnimation();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_purgeCachedData", [](auto L) -> int {
        _director->purgeCachedData();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setClearColor", [](auto L) -> int {
        _director->setClearColor({ XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) });
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setContentScaleFactor", [](auto L) -> int {
        _director->setContentScaleFactor(XL::To<float>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getContentScaleFactor", [](auto L) -> int {
        return XL::Push(L, _director->getContentScaleFactor());
    });

    XL::SetGlobalCClosure(L, "cc_getDeltaTime", [](auto L) -> int {
        return XL::Push(L, _director->getDeltaTime());
    });

    XL::SetGlobalCClosure(L, "cc_getFrameRate", [](auto L) -> int {
        return XL::Push(L, _director->getFrameRate());
    });

    /***********************************************************************************************/
    // GLView

    XL::SetGlobalCClosure(L, "cc_initGLView", [](auto L) -> int {
        _glView = _director->getOpenGLView();
        if (!_glView) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
            _glView = cocos2d::GLViewImpl::createWithRect(XL::To<std::string>(L, 1), cocos2d::Rect(0, 0, XL::To<float>(L, 2), XL::To<float>(L, 3)));
#else
            _glView = cocos2d::GLViewImpl::create(XL::To<std::string_view>(L, 1));
#endif
            cocos2d::Director::getInstance()->setOpenGLView(_glView);
        }
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setDesignResolutionSize", [](auto L) -> int {
        assert(_glView);
        _glView->setDesignResolutionSize(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<ResolutionPolicy>(L, 3));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getFrameSize", [](auto L) -> int {
        assert(_glView);
        auto&& r = _glView->getFrameSize();
        return XL::Push(L, r.x, r.y);
    });

    /***********************************************************************************************/
    // EventDispacher

    XL::SetGlobalCClosure(L, "cc_addEventListenerWithSceneGraphPriority", [](auto L) -> int {
        _eventDispatcher->addEventListenerWithSceneGraphPriority(XL::To<cocos2d::EventListener*>(L, 1), XL::To<cocos2d::Node*>(L, 2));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_addEventListenerWithFixedPriority", [](auto L) -> int {
        _eventDispatcher->addEventListenerWithFixedPriority(XL::To<cocos2d::EventListener*>(L, 1), XL::To<int>(L, 2));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_removeEventListener", [](auto L) -> int {
        _eventDispatcher->removeEventListener(XL::To<cocos2d::EventListener*>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_removeEventListenersForType", [](auto L) -> int {
        _eventDispatcher->removeEventListenersForType(XL::To<cocos2d::EventListener::Type>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_removeEventListenersForTarget", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 1: {
            _eventDispatcher->removeEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1));
            return 0;
        }
        case 2: {
            _eventDispatcher->removeEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1), XL::To<bool>(L, 2));
            return 0;
        }
        default: {
            return luaL_error(L, "%s", "cc_removeEventListenersForTarget error! need 1 ~ 2 args: Node target, bool recursive = false");
        }
        }
    });

    XL::SetGlobalCClosure(L, "cc_removeAllEventListeners", [](auto L) -> int {
        _eventDispatcher->removeAllEventListeners();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_pauseEventListenersForTarget", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 1: {
            _eventDispatcher->pauseEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1));
            return 0;
        }
        case 2: {
            _eventDispatcher->pauseEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1), XL::To<bool>(L, 2));
            return 0;
        }
        default: {
            return luaL_error(L, "%s", "cc_pauseEventListenersForTarget error! need 1 ~ 2 args: Node target, bool recursive = false");
        }
        }
    });

    XL::SetGlobalCClosure(L, "cc_resumeEventListenersForTarget", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 1: {
            _eventDispatcher->resumeEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1));
            return 0;
        }
        case 2: {
            _eventDispatcher->resumeEventListenersForTarget(XL::To<cocos2d::Node*>(L, 1), XL::To<bool>(L, 2));
            return 0;
        }
        default: {
            return luaL_error(L, "%s", "cc_resumeEventListenersForTarget error! need 1 ~ 2 args: Node target, bool recursive = false");
        }
        }
    });

    XL::SetGlobalCClosure(L, "cc_setPriority", [](auto L) -> int {
        _eventDispatcher->setPriority(XL::To<cocos2d::EventListener*>(L, 1), XL::To<int>(L, 2));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setEnabled", [](auto L) -> int {
        _eventDispatcher->setEnabled(XL::To<bool>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_isEnabled", [](auto L) -> int {
        return XL::Push(L, _eventDispatcher->isEnabled());
    });

    XL::SetGlobalCClosure(L, "cc_hasEventListener", [](auto L) -> int {
        return XL::Push(L, _eventDispatcher->hasEventListener(XL::To<std::string>(L, 1)));
    });



    /***********************************************************************************************/
    // FileUtils

    XL::SetGlobalCClosure(L, "cc_purgeCachedEntries", [](auto L) -> int {
        _fileUtils->purgeCachedEntries();
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getStringFromFile", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getStringFromFile(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_getDataFromFile", [](auto L) -> int {
        auto&& r = _fileUtils->getDataFromFile(XL::To<std::string>(L, 1));
        xx::Data d;
        // 将 r 的数据移动到 d 避免 copy ( 都用的 malloc free，故兼容 )
        d.Reset(r.getBytes(), r.getSize(), r.getSize());
        r.fastSet(nullptr, 0);
        // 继续移动 d 到 lua 内存
        return XL::Push(L, std::move(d));
    });

    XL::SetGlobalCClosure(L, "cc_fullPathForFilename", [](auto L) -> int {
        return XL::Push(L, _fileUtils->fullPathForFilename(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_fullPathFromRelativeFile", [](auto L) -> int {
        return XL::Push(L, _fileUtils->fullPathFromRelativeFile(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_setSearchPaths", [](auto L) -> int {
        _fileUtils->setSearchPaths(XL::To<std::vector<std::string>>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getDefaultResourceRootPath", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getDefaultResourceRootPath());
    });

    XL::SetGlobalCClosure(L, "cc_setDefaultResourceRootPath", [](auto L) -> int {
        _fileUtils->setDefaultResourceRootPath(XL::To<std::string>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_addSearchPath", [](auto L) -> int {
        _fileUtils->addSearchPath(XL::To<std::string>(L, 1), XL::To<bool>(L, 2));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getSearchPaths", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getSearchPaths());
    });

    XL::SetGlobalCClosure(L, "cc_getOriginalSearchPaths", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getOriginalSearchPaths());
    });

    XL::SetGlobalCClosure(L, "cc_getWritablePath", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getWritablePath());
    });

    XL::SetGlobalCClosure(L, "cc_setWritablePath", [](auto L) -> int {
        _fileUtils->setWritablePath(XL::To<std::string>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_setPopupNotify", [](auto L) -> int {
        _fileUtils->setPopupNotify(XL::To<bool>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_isPopupNotify", [](auto L) -> int {
        return XL::Push(L, _fileUtils->isPopupNotify());
    });

    XL::SetGlobalCClosure(L, "cc_writeStringToFile", [](auto L) -> int {
        return XL::Push(L, _fileUtils->writeStringToFile(/* data */XL::To<std::string>(L, 1), /* path */XL::To<std::string>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_writeDataToFile", [](auto L) -> int {
        auto a = XL::To<xx::Data*>(L, 1);
        // 借壳传数据
        cocos2d::Data d;
        d.fastSet(a->buf, a->len);
        auto&& r = XL::Push(L, _fileUtils->writeDataToFile(d, XL::To<std::string>(L, 2)));
        d.fastSet(nullptr, 0);
        return r;
    });

    XL::SetGlobalCClosure(L, "cc_isFileExist", [](auto L) -> int {
        return XL::Push(L, _fileUtils->isFileExist(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_getFileExtension", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getFileExtension(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_isAbsolutePath", [](auto L) -> int {
        return XL::Push(L, _fileUtils->isAbsolutePath(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_isDirectoryExist", [](auto L) -> int {
        return XL::Push(L, _fileUtils->isDirectoryExist(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_createDirectory", [](auto L) -> int {
        return XL::Push(L, _fileUtils->createDirectory(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_removeDirectory", [](auto L) -> int {
        return XL::Push(L, _fileUtils->removeDirectory(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_removeFile", [](auto L) -> int {
        return XL::Push(L, _fileUtils->removeFile(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_renameFile", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2:
            // oldFullPath, newFullPath
            _fileUtils->renameFile(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2));
            break;
        case 3:
            // path, oldName, newName
            _fileUtils->renameFile(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<std::string>(L, 3));
            break;
        }
        return luaL_error(L, "%s", "removeFile error! need 2 ~ 3 args");
    });

    XL::SetGlobalCClosure(L, "cc_getFileSize", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getFileSize(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_listFiles", [](auto L) -> int {
        return XL::Push(L, _fileUtils->listFiles(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_listFilesRecursively", [](auto L) -> int {
        _strings.clear();
        _fileUtils->listFilesRecursively(XL::To<std::string>(L, 1), &_strings);
        return XL::Push(L, _strings);
    });

    XL::SetGlobalCClosure(L, "cc_getFullPathCache", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getFullPathCache());
    });

    XL::SetGlobalCClosure(L, "cc_getNewFilename", [](auto L) -> int {
        return XL::Push(L, _fileUtils->getNewFilename(XL::To<std::string>(L, 1)));
    });



    /***********************************************************************************************/
    // AnimationCache

    XL::SetGlobalCClosure(L, "cc_addAnimation", [](auto L) -> int {
        _animationCache->addAnimation(XL::To<cocos2d::Animation*>(L, 1), XL::To<std::string>(L, 2));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_removeAnimation", [](auto L) -> int {
        _animationCache->removeAnimation(XL::To<std::string>(L, 1));
        return 0;
    });

    XL::SetGlobalCClosure(L, "cc_getAnimation", [](auto L) -> int {
        return XL::Push(L, _animationCache->getAnimation(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_addAnimationsWithFile", [](auto L) -> int {
        _animationCache->addAnimationsWithFile(XL::To<std::string>(L, 1));
        return 0;
    });

    



    /***********************************************************************************************/
    // 各种 create

    xx::Lua::Data::Register(L, "xx_data_create");

    // 各种 Action Animation

    XL::SetGlobalCClosure(L, "cc_Animation_create", [](auto L) -> int {
        auto&& numArgs = lua_gettop(L);
        if (!numArgs) return XL::Push(L, cocos2d::Animation::create());

        auto&& sg = xx::MakeScopeGuard([&] { _animationFrames2.clear(); });
        XL::To(L, 1, _animationFrames2);
        if (!_animationFrames2.empty()) {
            switch (numArgs) {
            case 2: {
                return XL::Push(L, cocos2d::Animation::create(_animationFrames2, XL::To<float>(L, 2)));
            }
            case 3: {
                return XL::Push(L, cocos2d::Animation::create(_animationFrames2, XL::To<float>(L, 2), XL::To<int>(L, 3)));
            }
            }
        }
        return luaL_error(L, "%s", "cc_Animation_create error! need 2 ~ 3 args: SpriteFrame[] arrayOfAnimationFrameNames, float delayPerUnit, loops = 1");
    });

    XL::SetGlobalCClosure(L, "cc_Animation_createWithSpriteFrames", [](auto L) -> int {
        auto&& numArgs = lua_gettop(L);
        if (numArgs) {
            auto&& sg = xx::MakeScopeGuard([&] { _spriteFrames2.clear(); });
            XL::To(L, 1, _spriteFrames2);
            if (!_spriteFrames2.empty()) {
                switch (numArgs) {
                case 1: {
                    return XL::Push(L, cocos2d::Animation::createWithSpriteFrames(_spriteFrames2));
                }
                case 2: {
                    return XL::Push(L, cocos2d::Animation::createWithSpriteFrames(_spriteFrames2, XL::To<float>(L, 2)));
                }
                case 3: {
                    return XL::Push(L, cocos2d::Animation::createWithSpriteFrames(_spriteFrames2, XL::To<float>(L, 2), XL::To<int>(L, 3)));
                }
                }
            }
        }
        return luaL_error(L, "%s", "cc_Animation_createWithSpriteFrames error! need 1 ~ 3 args: SpriteFrame[] frames, float delay = 0, loop = 1");
    });

    XL::SetGlobalCClosure(L, "cc_BezierBy_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::BezierBy::create(XL::To<float>(L, 1)
            , { { XL::To<float>(L, 2), XL::To<float>(L, 3)}, {XL::To<float>(L, 4), XL::To<float>(L, 5)}, {XL::To<float>(L, 6), XL::To<float>(L, 7)} }
        ));
    });

    XL::SetGlobalCClosure(L, "cc_BezierTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::BezierTo::create(XL::To<float>(L, 1)
            , { { XL::To<float>(L, 2), XL::To<float>(L, 3)}, {XL::To<float>(L, 4), XL::To<float>(L, 5)}, {XL::To<float>(L, 6), XL::To<float>(L, 7)} }
        ));
    });

    XL::SetGlobalCClosure(L, "cc_Blink_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Blink::create(XL::To<float>(L, 1), XL::To<int>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_CallFunc_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::CallFunc::create([f = XL::To<XL::Func>(L, 1)]{
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                if (auto&& r = XL::Try(_luaState, [&] {
#endif
                    f();
#if XX_LUA_ENABLE_TRY_CALL_FUNC
                })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
        }));
    });

    XL::SetGlobalCClosure(L, "cc_DelayTime_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::DelayTime::create(XL::To<float>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_FadeIn_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::FadeIn::create(XL::To<float>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_FadeOut_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::FadeOut::create(XL::To<float>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_FadeTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::FadeTo::create(XL::To<float>(L, 1), XL::To<uint8_t>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_FlipX_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::FlipX::create(XL::To<bool>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_FlipY_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::FlipY::create(XL::To<bool>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_Hide_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Hide::create());
    });

    XL::SetGlobalCClosure(L, "cc_JumpTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::JumpTo::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }, XL::To<float>(L, 4), XL::To<int>(L, 5)));
    });

    XL::SetGlobalCClosure(L, "cc_JumpBy_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::JumpBy::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }, XL::To<float>(L, 4), XL::To<int>(L, 5)));
    });

    XL::SetGlobalCClosure(L, "cc_MoveBy_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 3: {
            return XL::Push(L, cocos2d::MoveBy::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }));
        }
        case 4: {
            return XL::Push(L, cocos2d::MoveBy::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) }));
        }
        default:
            return luaL_error(L, "%s", "cc_MoveBy_create error! need 3 ~ 4 args: float duration, float deltaX, float deltaY, float deltaZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_MoveTo_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 3: {
            return XL::Push(L, cocos2d::MoveTo::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }));
        }
        case 4: {
            return XL::Push(L, cocos2d::MoveTo::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) }));
        }
        default:
            return luaL_error(L, "%s", "cc_MoveTo_create error! need 3 ~ 4 args: float duration, float dstX, float dstY, float dstZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Place_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Place::create({ XL::To<float>(L, 1), XL::To<float>(L, 2) }));
    });

    XL::SetGlobalCClosure(L, "cc_RemoveSelf_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::RemoveSelf::create());
    });

    XL::SetGlobalCClosure(L, "cc_Repeat_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Repeat::create(XL::To<cocos2d::FiniteTimeAction*>(L, 1), XL::To<int>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_RepeatForever_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::RepeatForever::create(XL::To<cocos2d::ActionInterval*>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_ResizeTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::ResizeTo::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }));
    });

    XL::SetGlobalCClosure(L, "cc_ResizeBy_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::ResizeBy::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3) }));
    });

    XL::SetGlobalCClosure(L, "cc_RotateTo_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2: {
            return XL::Push(L, cocos2d::RotateTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2)));
        }
        case 3: {
            return XL::Push(L, cocos2d::RotateTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
        }
        case 4: {
            return XL::Push(L, cocos2d::RotateTo::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) }));
        }
        default:
            return luaL_error(L, "%s", "cc_RotateTo_create error! need 2 ~ 4 args: float duration, float dstAngleX, float dstAngleY, float dstAngleZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_RotateBy_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2: {
            return XL::Push(L, cocos2d::RotateBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2)));
        }
        case 3: {
            return XL::Push(L, cocos2d::RotateBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
        }
        case 4: {
            return XL::Push(L, cocos2d::RotateBy::create(XL::To<float>(L, 1), { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) }));
        }
        default:
            return luaL_error(L, "%s", "cc_RotateBy_create error! need 2 ~ 4 args: float duration, float dstAngleX, float dstAngleY, float dstAngleZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_ScaleTo_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2: {
            return XL::Push(L, cocos2d::ScaleTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2)));
        }
        case 3: {
            return XL::Push(L, cocos2d::ScaleTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
        }
        case 4: {
            return XL::Push(L, cocos2d::ScaleTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4)));
        }
        default:
            return luaL_error(L, "%s", "cc_ScaleTo_create error! need 2 ~ 4 args: float duration, float sXY/sX, float sY, float sZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_ScaleBy_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2: {
            return XL::Push(L, cocos2d::ScaleBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2)));
        }
        case 3: {
            return XL::Push(L, cocos2d::ScaleBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
        }
        case 4: {
            return XL::Push(L, cocos2d::ScaleBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4)));
        }
        default:
            return luaL_error(L, "%s", "cc_ScaleBy_create error! need 2 ~ 4 args: float duration, float sXY/sX, float sY, float sZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Sequence_create", [](auto L) -> int {
        auto&& numArgs = lua_gettop(L);
        if (!numArgs) {
            return luaL_error(L, "%s", "; error! need 1+ args: FiniteTimeAction*... / { FiniteTimeAction*... }");
        }
        if (lua_istable(L, 1)) {
            XL::To(L, 1, _finiteTimeActions);
        }
        else {
            _finiteTimeActions.clear();
            for (int i = 1; i <= numArgs; ++i) {
                _finiteTimeActions.push_back(XL::To<cocos2d::FiniteTimeAction*>(L, i));
            }
        }
        if (_finiteTimeActions.size() == 2) {
            return XL::Push(L, cocos2d::Sequence::createWithTwoActions(_finiteTimeActions[0], _finiteTimeActions[1]));
        }
        else {
            auto&& sg = xx::MakeScopeGuard([&] { _finiteTimeActions2.clear(); });
            for (auto& o : _finiteTimeActions) _finiteTimeActions2.pushBack(o);
            return XL::Push(L, cocos2d::Sequence::create(_finiteTimeActions2));
        }
    });

    XL::SetGlobalCClosure(L, "cc_SkewTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::SkewTo::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
    });

    XL::SetGlobalCClosure(L, "cc_Show_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Show::create());
    });

    XL::SetGlobalCClosure(L, "cc_SkewBy_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::SkewBy::create(XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3)));
    });

    XL::SetGlobalCClosure(L, "cc_Spawn_create", [](auto L) -> int {
        auto&& numArgs = lua_gettop(L);
        if (!numArgs) {
            return luaL_error(L, "%s", "cc_Spawn_create error! need 1+ args: FiniteTimeAction*... / { FiniteTimeAction*... }");
        }
        auto&& sg = xx::MakeScopeGuard([&] { _finiteTimeActions2.clear(); });
        if (lua_istable(L, 1)) {
            XL::To(L, 1, _finiteTimeActions2);
        }
        else {
            for (int i = 1; i <= numArgs; ++i) {
                _finiteTimeActions2.pushBack(XL::To<cocos2d::FiniteTimeAction*>(L, i));
            }
        }
        return XL::Push(L, cocos2d::Spawn::create(_finiteTimeActions2));
    });

    XL::SetGlobalCClosure(L, "cc_Speed_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Speed::create(XL::To<cocos2d::ActionInterval*>(L, 1), XL::To<float>(L, 2)));
    });

    XL::SetGlobalCClosure(L, "cc_TintBy_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::TintBy::create(XL::To<float>(L, 1), XL::To<int16_t>(L, 2), XL::To<int16_t>(L, 3), XL::To<int16_t>(L, 4)));
    });

    XL::SetGlobalCClosure(L, "cc_TintTo_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::TintTo::create(XL::To<float>(L, 1), XL::To<uint8_t>(L, 2), XL::To<uint8_t>(L, 3), XL::To<uint8_t>(L, 4)));
    });

    XL::SetGlobalCClosure(L, "cc_ToggleVisibility_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::ToggleVisibility::create());
    });

    // 各种 EventListener

    XL::SetGlobalCClosure(L, "cc_EventListenerTouchAllAtOnce_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::EventListenerTouchAllAtOnce::create());
    });

    XL::SetGlobalCClosure(L, "cc_EventListenerKeyboard_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::EventListenerKeyboard::create());
    });

    XL::SetGlobalCClosure(L, "cc_EventListenerTouchOneByOne_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::EventListenerTouchOneByOne::create());
    });

    // 各种 Node

    XL::SetGlobalCClosure(L, "cc_Node_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Node::create());
    });

    XL::SetGlobalCClosure(L, "cc_Scene_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::create());
    });

    XL::SetGlobalCClosure(L, "cc_Scene_createWithSize", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::createWithSize({ XL::To<float>(L, 1), XL::To<float>(L, 2) }));
    });

    XL::SetGlobalCClosure(L, "cc_Sprite_create", [](auto L) -> int {
        auto&& r = lua_gettop(L)
            ? cocos2d::Sprite::create(XL::To<std::string>(L, 1))
            : cocos2d::Sprite::create();
        // todo: PolygonInfo, filename + rect
        return XL::Push(L, r);
    });

    XL::SetGlobalCClosure(L, "cc_Sprite_createWithSpriteFrame", [](auto L) -> int {
        return XL::Push(L, cocos2d::Sprite::createWithSpriteFrame(XL::To<cocos2d::SpriteFrame*>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_Sprite_createWithSpriteFrameName", [](auto L) -> int {
        return XL::Push(L, cocos2d::Sprite::createWithSpriteFrameName(XL::To<std::string>(L, 1)));
    });

    XL::SetGlobalCClosure(L, "cc_Sprite_createWithTexture", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 1: {
            return XL::Push(L, cocos2d::Sprite::createWithTexture(XL::To<cocos2d::Texture2D*>(L, 1)));
        }
        case 5: {
            return XL::Push(L, cocos2d::Sprite::createWithTexture(XL::To<cocos2d::Texture2D*>(L, 1)
                , { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) }));
        }
        case 6: {
            return XL::Push(L, cocos2d::Sprite::createWithTexture(XL::To<cocos2d::Texture2D*>(L, 1)
                , { XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4), XL::To<float>(L, 5) }, XL::To<bool>(L, 6)));
        }
        default:
            return luaL_error(L, "%s", "cc_Sprite_createWithTexture error! need 1 / 5 / 6 args: Texture2D texture, Rect{ float x, y, w, h }, bool rotated = false");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Label_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Label::create());
    });

    XL::SetGlobalCClosure(L, "cc_Label_createWithSystemFont", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 3: {
            return XL::Push(L, cocos2d::Label::createWithSystemFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)));
        }
        case 5: {
            return XL::Push(L, cocos2d::Label::createWithSystemFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }));
        }
        case 6: {
            return XL::Push(L, cocos2d::Label::createWithSystemFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }, XL::To<cocos2d::TextHAlignment>(L, 6)));
        }
        case 7: {
            return XL::Push(L, cocos2d::Label::createWithSystemFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }, XL::To<cocos2d::TextHAlignment>(L, 6), XL::To<cocos2d::TextVAlignment>(L, 7)));
        }
        default:
            return luaL_error(L, "%s", "cc_Label_createWithSystemFont error! need 3, 5 ~ 7 args: string text, font, float fontSize, dimensionsX = 0, dimensionsY = 0, TextHAlignment hAlignment = TextHAlignment::LEFT, TextVAlignment vAlignment = TextVAlignment::TOP");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Label_createWithTTF", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 3: {
            return XL::Push(L, cocos2d::Label::createWithTTF(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)));
        }
        case 5: {
            return XL::Push(L, cocos2d::Label::createWithTTF(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }));
        }
        case 6: {
            return XL::Push(L, cocos2d::Label::createWithTTF(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }, XL::To<cocos2d::TextHAlignment>(L, 6)));
        }
        case 7: {
            return XL::Push(L, cocos2d::Label::createWithTTF(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2), XL::To<float>(L, 3)
                , { XL::To<float>(L, 4), XL::To<float>(L, 5) }, XL::To<cocos2d::TextHAlignment>(L, 6), XL::To<cocos2d::TextVAlignment>(L, 7)));
        }
        default:
            return luaL_error(L, "%s", "cc_Label_createWithTTF error! need 3, 5 ~ 7 args: string text, fontFilePath, float fontSize, dimensionsX = 0, dimensionsY = 0, TextHAlignment hAlignment = TextHAlignment::LEFT, TextVAlignment vAlignment = TextVAlignment::TOP");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Label_createWithBMFont", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 2: {
            return XL::Push(L, cocos2d::Label::createWithBMFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)));
        }
        case 3: {
            return XL::Push(L, cocos2d::Label::createWithBMFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)
                , XL::To<cocos2d::TextHAlignment>(L, 3)));
        }
        case 4: {
            return XL::Push(L, cocos2d::Label::createWithBMFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)
                , XL::To<cocos2d::TextHAlignment>(L, 3), XL::To<int>(L, 4)));
        }
        case 5: {
            return XL::Push(L, cocos2d::Label::createWithBMFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)
                , XL::To<cocos2d::TextHAlignment>(L, 3), XL::To<int>(L, 4), XL::To<std::string>(L, 5)));
        }
        case 9: {
            return XL::Push(L, cocos2d::Label::createWithBMFont(XL::To<std::string>(L, 1), XL::To<std::string>(L, 2)
                , XL::To<cocos2d::TextHAlignment>(L, 3), XL::To<int>(L, 4)
                , { XL::To<float>(L, 5), XL::To<float>(L, 6), XL::To<float>(L, 7), XL::To<float>(L, 8) }, XL::To<bool>(L, 9)));
        }
        default:
            return luaL_error(L, "%s", "cc_Label_createWithBMFont error! need 2 ~ 5, 9 args: string bmfontPath, text, hAlignment = TextHAlignment::LEFT, maxLineWidth = 0, string subTextureKey / Rect{ float x, y, w, h }, bool imageRotated");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Label_createWithCharMap", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 1: {
            return XL::Push(L, cocos2d::Label::createWithCharMap(XL::To<std::string>(L, 1)));
        }
        case 4: {
            if (lua_isstring(L, 1)) return XL::Push(L, cocos2d::Label::createWithCharMap(XL::To<std::string>(L, 1), XL::To<int>(L, 2), XL::To<int>(L, 3), XL::To<int>(L, 4)));
            else return XL::Push(L, cocos2d::Label::createWithCharMap(XL::To<cocos2d::Texture2D*>(L, 1), XL::To<int>(L, 2), XL::To<int>(L, 3), XL::To<int>(L, 4)));
        }
        default:
            return luaL_error(L, "%s", "cc_Label_createWithCharMap error! need 1, 4 args: string plistFile | Texture2D* texture / string charMapFile, int itemWidth, int itemHeight, int startCharMap");
        }
    });

    XL::SetGlobalCClosure(L, "cc_ScrollView_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 0: {
            return XL::Push(L, cocos2d::extension::ScrollView::create());
        }
        case 2: {
            return XL::Push(L, cocos2d::extension::ScrollView::create({ XL::To<float>(L, 1), XL::To<float>(L, 2) }));
        }
        case 3: {
            return XL::Push(L, cocos2d::extension::ScrollView::create({ XL::To<float>(L, 1), XL::To<float>(L, 2) }, XL::To<cocos2d::Node*>(L, 3)));
        }
        default:
            return luaL_error(L, "%s", "cc_ScrollView_create error! need 0, 2, 3 args: Size size { float w, h }, Node* container = NULL");
        }
    });

    XL::SetGlobalCClosure(L, "cc_ClippingNode_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 0: {
            return XL::Push(L, cocos2d::ClippingNode::create());
        }
        case 1: {
            return XL::Push(L, cocos2d::ClippingNode::create(XL::To<cocos2d::Node*>(L, 1)));
        }
        default:
            return luaL_error(L, "%s", "cc_ClippingNode_create error! need 0, 1 args: Node *stencil");
        }
    });

    XL::SetGlobalCClosure(L, "cc_ClippingRectangleNode_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 0: {
            return XL::Push(L, cocos2d::ClippingRectangleNode::create());
        }
        case 4: {
            return XL::Push(L, cocos2d::ClippingRectangleNode::create({ XL::To<float>(L, 1), XL::To<float>(L, 2), XL::To<float>(L, 3), XL::To<float>(L, 4) }));
        }
        default:
            return luaL_error(L, "%s", "cc_ClippingRectangleNode_create error! need 0, 4 args: Rect region{ float x, y, w, h }");
        }
    });

    XL::SetGlobalCClosure(L, "cc_DrawNode_create", [](auto L) -> int {
        switch (lua_gettop(L)) {
        case 0: {
            return XL::Push(L, cocos2d::DrawNode::create());
        }
        case 1: {
            return XL::Push(L, cocos2d::DrawNode::create(XL::To<float>(L, 1)));
        }
        default:
            return luaL_error(L, "%s", "cc_DrawNode_create error! need 0, 1 args: float defaultLineWidth = DEFAULT_LINE_WIDTH");
        }
    });


    /***********************************************************************************************/
    // enums

    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_WINDOWS", cocos2d::ApplicationProtocol::Platform::OS_WINDOWS);
    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_LINUX", cocos2d::ApplicationProtocol::Platform::OS_LINUX);
    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_MAC", cocos2d::ApplicationProtocol::Platform::OS_MAC);
    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_ANDROID", cocos2d::ApplicationProtocol::Platform::OS_ANDROID);
    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_IPHONE", cocos2d::ApplicationProtocol::Platform::OS_IPHONE);
    XL::SetGlobal(L, "ApplicationProtocol_Platform_OS_IPAD", cocos2d::ApplicationProtocol::Platform::OS_IPAD);

    XL::SetGlobal(L, "Event_Type_TOUCH", cocos2d::Event::Type::TOUCH);
    XL::SetGlobal(L, "Event_Type_KEYBOARD", cocos2d::Event::Type::KEYBOARD);
    XL::SetGlobal(L, "Event_Type_ACCELERATION", cocos2d::Event::Type::ACCELERATION);
    XL::SetGlobal(L, "Event_Type_MOUSE", cocos2d::Event::Type::MOUSE);
    XL::SetGlobal(L, "Event_Type_FOCUS", cocos2d::Event::Type::FOCUS);
    XL::SetGlobal(L, "Event_Type_GAME_CONTROLLER", cocos2d::Event::Type::GAME_CONTROLLER);
    XL::SetGlobal(L, "Event_Type_CUSTOM", cocos2d::Event::Type::CUSTOM);

    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_NONE", cocos2d::EventKeyboard::KeyCode::KEY_NONE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PAUSE", cocos2d::EventKeyboard::KeyCode::KEY_PAUSE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SCROLL_LOCK", cocos2d::EventKeyboard::KeyCode::KEY_SCROLL_LOCK);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PRINT", cocos2d::EventKeyboard::KeyCode::KEY_PRINT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SYSREQ", cocos2d::EventKeyboard::KeyCode::KEY_SYSREQ);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BREAK", cocos2d::EventKeyboard::KeyCode::KEY_BREAK);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_ESCAPE", cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BACK", cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BACKSPACE", cocos2d::EventKeyboard::KeyCode::KEY_BACKSPACE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_TAB", cocos2d::EventKeyboard::KeyCode::KEY_TAB);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BACK_TAB", cocos2d::EventKeyboard::KeyCode::KEY_BACK_TAB);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RETURN", cocos2d::EventKeyboard::KeyCode::KEY_RETURN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPS_LOCK", cocos2d::EventKeyboard::KeyCode::KEY_CAPS_LOCK);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SHIFT", cocos2d::EventKeyboard::KeyCode::KEY_SHIFT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_SHIFT", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_SHIFT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_SHIFT", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_SHIFT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CTRL", cocos2d::EventKeyboard::KeyCode::KEY_CTRL);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_CTRL", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_CTRL);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_CTRL", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_CTRL);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_ALT", cocos2d::EventKeyboard::KeyCode::KEY_ALT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_ALT", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ALT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_ALT", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ALT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_MENU", cocos2d::EventKeyboard::KeyCode::KEY_MENU);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_HYPER", cocos2d::EventKeyboard::KeyCode::KEY_HYPER);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_INSERT", cocos2d::EventKeyboard::KeyCode::KEY_INSERT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_HOME", cocos2d::EventKeyboard::KeyCode::KEY_HOME);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PG_UP", cocos2d::EventKeyboard::KeyCode::KEY_PG_UP);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DELETE", cocos2d::EventKeyboard::KeyCode::KEY_DELETE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_END", cocos2d::EventKeyboard::KeyCode::KEY_END);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PG_DOWN", cocos2d::EventKeyboard::KeyCode::KEY_PG_DOWN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_ARROW", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_ARROW", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_UP_ARROW", cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DOWN_ARROW", cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_NUM_LOCK", cocos2d::EventKeyboard::KeyCode::KEY_NUM_LOCK);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_PLUS", cocos2d::EventKeyboard::KeyCode::KEY_KP_PLUS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_MINUS", cocos2d::EventKeyboard::KeyCode::KEY_KP_MINUS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_MULTIPLY", cocos2d::EventKeyboard::KeyCode::KEY_KP_MULTIPLY);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_DIVIDE", cocos2d::EventKeyboard::KeyCode::KEY_KP_DIVIDE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_ENTER", cocos2d::EventKeyboard::KeyCode::KEY_KP_ENTER);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_HOME", cocos2d::EventKeyboard::KeyCode::KEY_KP_HOME);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_UP", cocos2d::EventKeyboard::KeyCode::KEY_KP_UP);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_PG_UP", cocos2d::EventKeyboard::KeyCode::KEY_KP_PG_UP);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_LEFT", cocos2d::EventKeyboard::KeyCode::KEY_KP_LEFT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_FIVE", cocos2d::EventKeyboard::KeyCode::KEY_KP_FIVE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_RIGHT", cocos2d::EventKeyboard::KeyCode::KEY_KP_RIGHT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_END", cocos2d::EventKeyboard::KeyCode::KEY_KP_END);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_DOWN", cocos2d::EventKeyboard::KeyCode::KEY_KP_DOWN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_PG_DOWN", cocos2d::EventKeyboard::KeyCode::KEY_KP_PG_DOWN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_INSERT", cocos2d::EventKeyboard::KeyCode::KEY_KP_INSERT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_KP_DELETE", cocos2d::EventKeyboard::KeyCode::KEY_KP_DELETE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F1", cocos2d::EventKeyboard::KeyCode::KEY_F1);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F2", cocos2d::EventKeyboard::KeyCode::KEY_F2);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F3", cocos2d::EventKeyboard::KeyCode::KEY_F3);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F4", cocos2d::EventKeyboard::KeyCode::KEY_F4);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F5", cocos2d::EventKeyboard::KeyCode::KEY_F5);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F6", cocos2d::EventKeyboard::KeyCode::KEY_F6);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F7", cocos2d::EventKeyboard::KeyCode::KEY_F7);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F8", cocos2d::EventKeyboard::KeyCode::KEY_F8);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F9", cocos2d::EventKeyboard::KeyCode::KEY_F9);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F10", cocos2d::EventKeyboard::KeyCode::KEY_F10);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F11", cocos2d::EventKeyboard::KeyCode::KEY_F11);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F12", cocos2d::EventKeyboard::KeyCode::KEY_F12);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SPACE", cocos2d::EventKeyboard::KeyCode::KEY_SPACE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_EXCLAM", cocos2d::EventKeyboard::KeyCode::KEY_EXCLAM);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_QUOTE", cocos2d::EventKeyboard::KeyCode::KEY_QUOTE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_NUMBER", cocos2d::EventKeyboard::KeyCode::KEY_NUMBER);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DOLLAR", cocos2d::EventKeyboard::KeyCode::KEY_DOLLAR);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PERCENT", cocos2d::EventKeyboard::KeyCode::KEY_PERCENT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CIRCUMFLEX", cocos2d::EventKeyboard::KeyCode::KEY_CIRCUMFLEX);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_AMPERSAND", cocos2d::EventKeyboard::KeyCode::KEY_AMPERSAND);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_APOSTROPHE", cocos2d::EventKeyboard::KeyCode::KEY_APOSTROPHE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_PARENTHESIS", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_PARENTHESIS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_PARENTHESIS", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_PARENTHESIS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_ASTERISK", cocos2d::EventKeyboard::KeyCode::KEY_ASTERISK);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PLUS", cocos2d::EventKeyboard::KeyCode::KEY_PLUS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_COMMA", cocos2d::EventKeyboard::KeyCode::KEY_COMMA);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_MINUS", cocos2d::EventKeyboard::KeyCode::KEY_MINUS);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PERIOD", cocos2d::EventKeyboard::KeyCode::KEY_PERIOD);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SLASH", cocos2d::EventKeyboard::KeyCode::KEY_SLASH);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_0", cocos2d::EventKeyboard::KeyCode::KEY_0);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_1", cocos2d::EventKeyboard::KeyCode::KEY_1);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_2", cocos2d::EventKeyboard::KeyCode::KEY_2);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_3", cocos2d::EventKeyboard::KeyCode::KEY_3);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_4", cocos2d::EventKeyboard::KeyCode::KEY_4);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_5", cocos2d::EventKeyboard::KeyCode::KEY_5);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_6", cocos2d::EventKeyboard::KeyCode::KEY_6);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_7", cocos2d::EventKeyboard::KeyCode::KEY_7);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_8", cocos2d::EventKeyboard::KeyCode::KEY_8);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_9", cocos2d::EventKeyboard::KeyCode::KEY_9);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_COLON", cocos2d::EventKeyboard::KeyCode::KEY_COLON);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SEMICOLON", cocos2d::EventKeyboard::KeyCode::KEY_SEMICOLON);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LESS_THAN", cocos2d::EventKeyboard::KeyCode::KEY_LESS_THAN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_EQUAL", cocos2d::EventKeyboard::KeyCode::KEY_EQUAL);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_GREATER_THAN", cocos2d::EventKeyboard::KeyCode::KEY_GREATER_THAN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_QUESTION", cocos2d::EventKeyboard::KeyCode::KEY_QUESTION);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_AT", cocos2d::EventKeyboard::KeyCode::KEY_AT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_A", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_A);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_B", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_B);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_C", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_C);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_D", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_D);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_E", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_E);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_F", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_F);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_G", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_G);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_H", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_H);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_I", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_I);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_J", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_J);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_K", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_K);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_L", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_L);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_M", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_M);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_N", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_N);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_O", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_O);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_P", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_P);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_Q", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Q);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_R", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_R);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_S", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_S);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_T", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_T);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_U", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_U);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_V", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_V);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_W", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_W);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_X", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_X);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_Y", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Y);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_CAPITAL_Z", cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Z);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_BRACKET", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_BRACKET);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BACK_SLASH", cocos2d::EventKeyboard::KeyCode::KEY_BACK_SLASH);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_BRACKET", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_BRACKET);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_UNDERSCORE", cocos2d::EventKeyboard::KeyCode::KEY_UNDERSCORE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_GRAVE", cocos2d::EventKeyboard::KeyCode::KEY_GRAVE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_A", cocos2d::EventKeyboard::KeyCode::KEY_A);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_B", cocos2d::EventKeyboard::KeyCode::KEY_B);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_C", cocos2d::EventKeyboard::KeyCode::KEY_C);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_D", cocos2d::EventKeyboard::KeyCode::KEY_D);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_E", cocos2d::EventKeyboard::KeyCode::KEY_E);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_F", cocos2d::EventKeyboard::KeyCode::KEY_F);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_G", cocos2d::EventKeyboard::KeyCode::KEY_G);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_H", cocos2d::EventKeyboard::KeyCode::KEY_H);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_I", cocos2d::EventKeyboard::KeyCode::KEY_I);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_J", cocos2d::EventKeyboard::KeyCode::KEY_J);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_K", cocos2d::EventKeyboard::KeyCode::KEY_K);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_L", cocos2d::EventKeyboard::KeyCode::KEY_L);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_M", cocos2d::EventKeyboard::KeyCode::KEY_M);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_N", cocos2d::EventKeyboard::KeyCode::KEY_N);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_O", cocos2d::EventKeyboard::KeyCode::KEY_O);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_P", cocos2d::EventKeyboard::KeyCode::KEY_P);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_Q", cocos2d::EventKeyboard::KeyCode::KEY_Q);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_R", cocos2d::EventKeyboard::KeyCode::KEY_R);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_S", cocos2d::EventKeyboard::KeyCode::KEY_S);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_T", cocos2d::EventKeyboard::KeyCode::KEY_T);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_U", cocos2d::EventKeyboard::KeyCode::KEY_U);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_V", cocos2d::EventKeyboard::KeyCode::KEY_V);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_W", cocos2d::EventKeyboard::KeyCode::KEY_W);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_X", cocos2d::EventKeyboard::KeyCode::KEY_X);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_Y", cocos2d::EventKeyboard::KeyCode::KEY_Y);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_Z", cocos2d::EventKeyboard::KeyCode::KEY_Z);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_LEFT_BRACE", cocos2d::EventKeyboard::KeyCode::KEY_LEFT_BRACE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_BAR", cocos2d::EventKeyboard::KeyCode::KEY_BAR);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_RIGHT_BRACE", cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_BRACE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_TILDE", cocos2d::EventKeyboard::KeyCode::KEY_TILDE);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_EURO", cocos2d::EventKeyboard::KeyCode::KEY_EURO);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_POUND", cocos2d::EventKeyboard::KeyCode::KEY_POUND);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_YEN", cocos2d::EventKeyboard::KeyCode::KEY_YEN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_MIDDLE_DOT", cocos2d::EventKeyboard::KeyCode::KEY_MIDDLE_DOT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_SEARCH", cocos2d::EventKeyboard::KeyCode::KEY_SEARCH);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DPAD_LEFT", cocos2d::EventKeyboard::KeyCode::KEY_DPAD_LEFT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DPAD_RIGHT", cocos2d::EventKeyboard::KeyCode::KEY_DPAD_RIGHT);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DPAD_UP", cocos2d::EventKeyboard::KeyCode::KEY_DPAD_UP);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DPAD_DOWN", cocos2d::EventKeyboard::KeyCode::KEY_DPAD_DOWN);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_DPAD_CENTER", cocos2d::EventKeyboard::KeyCode::KEY_DPAD_CENTER);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_ENTER", cocos2d::EventKeyboard::KeyCode::KEY_ENTER);
    XL::SetGlobal(L, "EventKeyboard_KeyCode_KEY_PLAY", cocos2d::EventKeyboard::KeyCode::KEY_PLAY);

    XL::SetGlobal(L, "EventListener_Type_UNKNOWN", cocos2d::EventListener::Type::UNKNOWN);
    XL::SetGlobal(L, "EventListener_Type_TOUCH_ONE_BY_ONE", cocos2d::EventListener::Type::TOUCH_ONE_BY_ONE);
    XL::SetGlobal(L, "EventListener_Type_TOUCH_ALL_AT_ONCE", cocos2d::EventListener::Type::TOUCH_ALL_AT_ONCE);
    XL::SetGlobal(L, "EventListener_Type_KEYBOARD", cocos2d::EventListener::Type::KEYBOARD);
    XL::SetGlobal(L, "EventListener_Type_MOUSE", cocos2d::EventListener::Type::MOUSE);
    XL::SetGlobal(L, "EventListener_Type_ACCELERATION", cocos2d::EventListener::Type::ACCELERATION);
    XL::SetGlobal(L, "EventListener_Type_FOCUS", cocos2d::EventListener::Type::FOCUS);
    XL::SetGlobal(L, "EventListener_Type_GAME_CONTROLLER", cocos2d::EventListener::Type::GAME_CONTROLLER);
    XL::SetGlobal(L, "EventListener_Type_CUSTOM", cocos2d::EventListener::Type::CUSTOM);

    XL::SetGlobal(L, "GlyphCollection_DYNAMIC", cocos2d::GlyphCollection::DYNAMIC);
    XL::SetGlobal(L, "GlyphCollection_NEHE", cocos2d::GlyphCollection::NEHE);
    XL::SetGlobal(L, "GlyphCollection_ASCII", cocos2d::GlyphCollection::ASCII);
    XL::SetGlobal(L, "GlyphCollection_CUSTOM", cocos2d::GlyphCollection::CUSTOM);

    XL::SetGlobal(L, "Label_Overflow_NONE", cocos2d::Label::Overflow::NONE);
    XL::SetGlobal(L, "Label_Overflow_CLAMP", cocos2d::Label::Overflow::CLAMP);
    XL::SetGlobal(L, "Label_Overflow_SHRINK", cocos2d::Label::Overflow::SHRINK);
    XL::SetGlobal(L, "Label_Overflow_RESIZE_HEIGHT", cocos2d::Label::Overflow::RESIZE_HEIGHT);

    XL::SetGlobal(L, "Label_Label_LabelType_TTF", cocos2d::Label::LabelType::TTF);
    XL::SetGlobal(L, "Label_Label_LabelType_BMFONT", cocos2d::Label::LabelType::BMFONT);
    XL::SetGlobal(L, "Label_Label_LabelType_CHARMAP", cocos2d::Label::LabelType::CHARMAP);
    XL::SetGlobal(L, "Label_Label_LabelType_STRING_TEXTURE", cocos2d::Label::LabelType::STRING_TEXTURE);

    XL::SetGlobal(L, "LabelEffect_NORMAL", cocos2d::LabelEffect::NORMAL);
    XL::SetGlobal(L, "LabelEffect_OUTLINE", cocos2d::LabelEffect::OUTLINE);
    XL::SetGlobal(L, "LabelEffect_SHADOW", cocos2d::LabelEffect::SHADOW);
    XL::SetGlobal(L, "LabelEffect_GLOW", cocos2d::LabelEffect::GLOW);
    XL::SetGlobal(L, "LabelEffect_ITALICS", cocos2d::LabelEffect::ITALICS);
    XL::SetGlobal(L, "LabelEffect_BOLD", cocos2d::LabelEffect::BOLD);
    XL::SetGlobal(L, "LabelEffect_UNDERLINE", cocos2d::LabelEffect::UNDERLINE);
    XL::SetGlobal(L, "LabelEffect_STRIKETHROUGH", cocos2d::LabelEffect::STRIKETHROUGH);
    XL::SetGlobal(L, "LabelEffect_ALL", cocos2d::LabelEffect::ALL);

    XL::SetGlobal(L, "LanguageType_ENGLISH", cocos2d::LanguageType::ENGLISH);
    XL::SetGlobal(L, "LanguageType_CHINESE", cocos2d::LanguageType::CHINESE);
    XL::SetGlobal(L, "LanguageType_FRENCH", cocos2d::LanguageType::FRENCH);
    XL::SetGlobal(L, "LanguageType_ITALIAN", cocos2d::LanguageType::ITALIAN);
    XL::SetGlobal(L, "LanguageType_GERMAN", cocos2d::LanguageType::GERMAN);
    XL::SetGlobal(L, "LanguageType_SPANISH", cocos2d::LanguageType::SPANISH);
    XL::SetGlobal(L, "LanguageType_DUTCH", cocos2d::LanguageType::DUTCH);
    XL::SetGlobal(L, "LanguageType_RUSSIAN", cocos2d::LanguageType::RUSSIAN);
    XL::SetGlobal(L, "LanguageType_KOREAN", cocos2d::LanguageType::KOREAN);
    XL::SetGlobal(L, "LanguageType_JAPANESE", cocos2d::LanguageType::JAPANESE);
    XL::SetGlobal(L, "LanguageType_HUNGARIAN", cocos2d::LanguageType::HUNGARIAN);
    XL::SetGlobal(L, "LanguageType_PORTUGUESE", cocos2d::LanguageType::PORTUGUESE);
    XL::SetGlobal(L, "LanguageType_ARABIC", cocos2d::LanguageType::ARABIC);
    XL::SetGlobal(L, "LanguageType_NORWEGIAN", cocos2d::LanguageType::NORWEGIAN);
    XL::SetGlobal(L, "LanguageType_POLISH", cocos2d::LanguageType::POLISH);
    XL::SetGlobal(L, "LanguageType_TURKISH", cocos2d::LanguageType::TURKISH);
    XL::SetGlobal(L, "LanguageType_UKRAINIAN", cocos2d::LanguageType::UKRAINIAN);
    XL::SetGlobal(L, "LanguageType_ROMANIAN", cocos2d::LanguageType::ROMANIAN);
    XL::SetGlobal(L, "LanguageType_BULGARIAN", cocos2d::LanguageType::BULGARIAN);
    XL::SetGlobal(L, "LanguageType_BELARUSIAN", cocos2d::LanguageType::BELARUSIAN);

    XL::SetGlobal(L, "Projection_2D", cocos2d::Director::Projection::_2D);
    XL::SetGlobal(L, "Projection_3D", cocos2d::Director::Projection::_3D);
    XL::SetGlobal(L, "Projection_CUSTOM", cocos2d::Director::Projection::CUSTOM);
    XL::SetGlobal(L, "Projection_DEFAULT", cocos2d::Director::Projection::DEFAULT);

    XL::SetGlobal(L, "ResolutionPolicy_EXACT_FIT", ResolutionPolicy::EXACT_FIT);
    XL::SetGlobal(L, "ResolutionPolicy_NO_BORDER", ResolutionPolicy::NO_BORDER);
    XL::SetGlobal(L, "ResolutionPolicy_SHOW_ALL", ResolutionPolicy::SHOW_ALL);
    XL::SetGlobal(L, "ResolutionPolicy_FIXED_HEIGHT", ResolutionPolicy::FIXED_HEIGHT);
    XL::SetGlobal(L, "ResolutionPolicy_FIXED_WIDTH", ResolutionPolicy::FIXED_WIDTH);
    XL::SetGlobal(L, "ResolutionPolicy_UNKNOWN", ResolutionPolicy::UNKNOWN);

    XL::SetGlobal(L, "ScrollView_Direction_NONE", cocos2d::extension::ScrollView::Direction::NONE);
    XL::SetGlobal(L, "ScrollView_Direction_HORIZONTAL", cocos2d::extension::ScrollView::Direction::HORIZONTAL);
    XL::SetGlobal(L, "ScrollView_Direction_VERTICAL", cocos2d::extension::ScrollView::Direction::VERTICAL);
    XL::SetGlobal(L, "ScrollView_Direction_BOTH", cocos2d::extension::ScrollView::Direction::BOTH);

    XL::SetGlobal(L, "TextHAlignment", cocos2d::TextHAlignment::LEFT);
    XL::SetGlobal(L, "TextHAlignment", cocos2d::TextHAlignment::CENTER);
    XL::SetGlobal(L, "TextHAlignment", cocos2d::TextHAlignment::RIGHT);

    XL::SetGlobal(L, "TextVAlignment", cocos2d::TextVAlignment::TOP);
    XL::SetGlobal(L, "TextVAlignment", cocos2d::TextVAlignment::CENTER);
    XL::SetGlobal(L, "TextVAlignment", cocos2d::TextVAlignment::BOTTOM);

    XL::SetGlobal(L, "Touch_DispatchMode_ALL_AT_ONCE", cocos2d::Touch::DispatchMode::ALL_AT_ONCE);
    XL::SetGlobal(L, "Touch_DispatchMode_ONE_BY_ONE", cocos2d::Touch::DispatchMode::ONE_BY_ONE);

    // todo: more enums


    // 在 cocos 中注册 lua 帧回调
    _scheduler->schedule([](float delta) {
        if (_globalUpdate) {
#if XX_LUA_ENABLE_TRY_CALL_FUNC
            if (auto&& r = XL::Try(_luaState, [&] {
#endif
                _globalUpdate(delta);
#if XX_LUA_ENABLE_TRY_CALL_FUNC
            })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
        }
    }, (void*)ad, 0, false, "AppDelegate");

    XL::AssertTop(L, 0);


    // 执行 main.lua
#if XX_LUA_ENABLE_TRY_CALL_FUNC
    if (auto&& r = XL::Try(L, [&] {
#endif
        xx::Lua::DoFile(L, "main.lua");
#if XX_LUA_ENABLE_TRY_CALL_FUNC
    })) { cocos2d::log("!!!! cpp call lua error !!!! file: %s line: %d err: %s", __FILE__, __LINE__, r.m.c_str()); }
#endif
}
