// 针对 adxe engine 的纯 cpp 导出项目的 lua 扩展
// 用法：
// 在 AppDelegate.cpp 第 34 行，也就是 一堆 include 之后，USING_NS_CC 之前 插入本 .h 的 include
// 并于 applicationDidFinishLaunching 的最后 执行 luaBinds(this);

#include <xx_lua_data.h>
namespace XL = xx::Lua;

// 单例区
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
inline static std::string _string;                                                  // for print
inline static std::vector<std::string> _strings;                                    // tmp
inline static std::vector<cocos2d::FiniteTimeAction*> _finiteTimeActions;           // tmp
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
    // 这个类的子类，需要自己实现返回具体类型的 clone 函数。直接压 Cloneable* 到 lua 没啥用，还需要转换一次才能用
    // SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, (TTTTTTTTTTTTTTTTTT*)(To<U>(L)->clone())); });

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
            SetFieldCClosure(L, "clone", [](auto L)->int { return Push(L, To<U>(L)->clone()); });
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
            // 不实现 clone. 因为是空的
            // 不实现 reverse. 因为是空的
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
            SetFieldCClosure(L, "execute", [](auto L)->int { To<U>(L)->execute(); return 0; });
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
        }
    };

    // todo: EaseXXXXXX ......

    // FadeTo : ActionInterval
    template<>
    struct MetaFuncs<cocos2d::FadeTo*, void> {
        using U = cocos2d::FadeTo*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::ActionInterval*>::Fill(L);
            SetType<U>(L);
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
            SetFieldCClosure(L, "getInnerAction", [](auto L)->int { return Push(L, To<U>(L)->getInnerAction()); });
            SetFieldCClosure(L, "setInnerAction", [](auto L)->int { To<U>(L)->setInnerAction(To<cocos2d::FiniteTimeAction*>(L, 2)); return 0; });
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
        }
    };

    // todo: 各种 action


    /*******************************************************************************************/
    // Event : Ref

    // todo




    /*******************************************************************************************/
    //EventDispatcher : Ref
    template<>
    struct MetaFuncs<cocos2d::EventDispatcher*, void> {
        using U = cocos2d::EventDispatcher*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
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
                    return luaL_error(L, "addChild error! need 2 ~ 4 args: self, Node child, int localZOrder, int tag/string name");
                }
            });
            SetFieldCClosure(L, "getChildByTag", [](auto L)->int { return Push(L, To<U>(L)->getChildByTag(XL::To<int>(L, 2))); });
            SetFieldCClosure(L, "getChildByName", [](auto L)->int { return Push(L, To<U>(L)->getChildByName(XL::To<std::string>(L, 2))); });
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
                    return luaL_error(L, "removeChild error! need 2 ~ 3 args: self, Node child, bool cleanup = true");
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
                    return luaL_error(L, "removeChildByTag error! need 2 ~ 3 args: self, int tag, bool cleanup = true");
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
                    return luaL_error(L, "removeChildByName error! need 2 ~ 3 args: self, string name, bool cleanup = true");
                }
            });
            SetFieldCClosure(L, "removeAllChildren", [](auto L)->int { To<U>(L)->removeAllChildren(); return 0; });
            SetFieldCClosure(L, "removeAllChildrenWithCleanup", [](auto L)->int { To<U>(L)->removeAllChildrenWithCleanup(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "reorderChild", [](auto L)->int { To<U>(L)->reorderChild(To<cocos2d::Node*>(L, 2), To<int>(L, 3)); return 0; });
            SetFieldCClosure(L, "sortAllChildren", [](auto L)->int { To<U>(L)->sortAllChildren(); return 0; });
            SetFieldCClosure(L, "setPositionNormalized", [](auto L)->int { To<U>(L)->setPositionNormalized({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getPositionNormalized", [](auto L)->int {
                auto&& r = To<U>(L)->getPositionNormalized();
                return Push(L, r.x, r.y);
            });
            SetFieldCClosure(L, "setPosition", [](auto L)->int { To<U>(L)->setPosition(To<float>(L, 2), To<float>(L, 3)); return 0; });
            SetFieldCClosure(L, "getPosition", [](auto L)->int { auto&& o = To<U>(L)->getPosition(); return Push(L, o.x, o.y); });
            SetFieldCClosure(L, "setPositionX", [](auto L)->int { To<U>(L)->setPositionX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionX", [](auto L)->int { return Push(L, To<U>(L)->getPositionX()); });
            SetFieldCClosure(L, "setPositionY", [](auto L)->int { To<U>(L)->setPositionY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionY", [](auto L)->int { return Push(L, To<U>(L)->getPositionY()); });
            SetFieldCClosure(L, "setPositionZ", [](auto L)->int { To<U>(L)->setPositionZ(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getPositionZ", [](auto L)->int { return Push(L, To<U>(L)->getPositionZ()); });
            SetFieldCClosure(L, "setPosition3D", [](auto L)->int { To<U>(L)->setPosition3D({ To<float>(L, 2), To<float>(L, 3), To<float>(L, 4) }); return 0; });
            SetFieldCClosure(L, "getPosition3D", [](auto L)->int { auto&& o = To<U>(L)->getPosition3D(); return Push(L, o.x, o.y, o.z); });
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
            SetFieldCClosure(L, "setLocalZOrder", [](auto L)->int { To<U>(L)->setLocalZOrder(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getLocalZOrder", [](auto L)->int { return Push(L, To<U>(L)->getLocalZOrder()); });
            SetFieldCClosure(L, "setGlobalZOrder", [](auto L)->int { To<U>(L)->setGlobalZOrder(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getGlobalZOrder", [](auto L)->int { return Push(L, To<U>(L)->getGlobalZOrder()); });
            SetFieldCClosure(L, "setRotationSkewX", [](auto L)->int { To<U>(L)->setRotationSkewX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getRotationSkewX", [](auto L)->int { return Push(L, To<U>(L)->getRotationSkewX()); });
            SetFieldCClosure(L, "setRotationSkewY", [](auto L)->int { To<U>(L)->setRotationSkewY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getRotationSkewY", [](auto L)->int { return Push(L, To<U>(L)->getRotationSkewY()); });
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
                    return luaL_error(L, "setScale error! need 2 ~ 3 args: self, float scaleX+Y / scaleX, scaleY");
                }
            });
            SetFieldCClosure(L, "setScaleX", [](auto L)->int { To<U>(L)->setScaleX(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleX", [](auto L)->int { return Push(L, To<U>(L)->getScaleX()); });
            SetFieldCClosure(L, "setScaleY", [](auto L)->int { To<U>(L)->setScaleY(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleY", [](auto L)->int { return Push(L, To<U>(L)->getScaleY()); });
            SetFieldCClosure(L, "setScaleZ", [](auto L)->int { To<U>(L)->setScaleZ(To<float>(L, 2)); return 0; });
            SetFieldCClosure(L, "getScaleZ", [](auto L)->int { return Push(L, To<U>(L)->getScaleZ()); });
            SetFieldCClosure(L, "setVisible", [](auto L)->int { To<U>(L)->setVisible(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isVisible", [](auto L)->int { return Push(L, To<U>(L)->isVisible()); });
            SetFieldCClosure(L, "setIgnoreAnchorPointForPosition", [](auto L)->int { To<U>(L)->setIgnoreAnchorPointForPosition(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isIgnoreAnchorPointForPosition", [](auto L)->int { return Push(L, To<U>(L)->isIgnoreAnchorPointForPosition()); });
            SetFieldCClosure(L, "setContentSize", [](auto L)->int { To<U>(L)->setContentSize({ To<float>(L, 2), To<float>(L, 3) }); return 0; });
            SetFieldCClosure(L, "getContentSize", [](auto L)->int {
                auto&& r = To<U>(L)->getContentSize();
                return Push(L, r.width, r.height);
            });
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
            SetFieldCClosure(L, "setOpacity", [](auto L)->int { To<U>(L)->setOpacity(To<uint8_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "getOpacity", [](auto L)->int { return Push(L, To<U>(L)->getOpacity()); });
            SetFieldCClosure(L, "updateDisplayedOpacity", [](auto L)->int { To<U>(L)->updateDisplayedOpacity(To<uint8_t>(L, 2)); return 0; });
            SetFieldCClosure(L, "getDisplayedOpacity", [](auto L)->int { return Push(L, To<U>(L)->getDisplayedOpacity()); });
            SetFieldCClosure(L, "setCascadeOpacityEnabled", [](auto L)->int { To<U>(L)->setCascadeOpacityEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isCascadeOpacityEnabled", [](auto L)->int { return Push(L, To<U>(L)->isCascadeOpacityEnabled()); });
            SetFieldCClosure(L, "setColor", [](auto L)->int { To<U>(L)->setColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4) }); return 0; });
            SetFieldCClosure(L, "getColor", [](auto L)->int {
                auto&& o = To<U>(L)->getColor(); 
                return Push(L, o.r, o.g, o.b);
            });
            SetFieldCClosure(L, "updateDisplayedColor", [](auto L)->int { To<U>(L)->updateDisplayedColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4) }); return 0; });
            SetFieldCClosure(L, "getDisplayedColor", [](auto L)->int {
                auto&& o = To<U>(L)->getDisplayedColor();
                return Push(L, o.r, o.g, o.b);
            });
            SetFieldCClosure(L, "setCascadeColorEnabled", [](auto L)->int { To<U>(L)->setCascadeColorEnabled(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isCascadeColorEnabled", [](auto L)->int { return Push(L, To<U>(L)->isCascadeColorEnabled()); });
            SetFieldCClosure(L, "setOpacityModifyRGB", [](auto L)->int { To<U>(L)->setOpacityModifyRGB(To<bool>(L, 2)); return 0; });
            SetFieldCClosure(L, "isOpacityModifyRGB", [](auto L)->int { return Push(L, To<U>(L)->isOpacityModifyRGB()); });
            SetFieldCClosure(L, "setOnEnterCallback", [](auto L)->int {
                To<U>(L)->setOnEnterCallback([f = To<Func>(L, 2)] {
                    if (auto&& r = Try(_luaState, [&] { f(); })) { xx::CoutN(r.m); }
                });
                return 0;
            });
            SetFieldCClosure(L, "setOnExitCallback", [](auto L)->int {
                To<U>(L)->setOnExitCallback([f = To<Func>(L, 2)] {
                    if (auto&& r = Try(_luaState, [&] { f(); })) { xx::CoutN(r.m); }
                });
                return 0;
            });
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
                    return luaL_error(L, "setCameraMask error! need 2 ~ 3 args: self, unsigned short mask, bool applyChildren = true");
                }
            });
            SetFieldCClosure(L, "getCameraMask", [](auto L)->int { return Push(L, To<U>(L)->getCameraMask()); });
            SetFieldCClosure(L, "setTag", [](auto L)->int { To<U>(L)->setTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getTag", [](auto L)->int { return Push(L, To<U>(L)->getTag()); });
            SetFieldCClosure(L, "setName", [](auto L)->int { To<U>(L)->setName(To<std::string>(L, 2)); return 0; });
            SetFieldCClosure(L, "getName", [](auto L)->int { return Push(L, To<U>(L)->getName()); });
            SetFieldCClosure(L, "cleanup", [](auto L)->int { To<U>(L)->cleanup(); return 0; });
            SetFieldCClosure(L, "getScene", [](auto L)->int { return Push(L, To<U>(L)->getScene()); });
            SetFieldCClosure(L, "getBoundingBox", [](auto L)->int {
                auto&& r = To<U>(L)->getBoundingBox();
                return Push(L, r.origin.x, r.origin.y, r.size.width, r.size.height);
            });
            SetFieldCClosure(L, "schedule", [](auto L)->int {
                To<U>(L)->schedule([f = To<Func>(L, 2)](float delta) {
                    if (auto&& r = Try(_luaState, [&] { f(delta); })) { xx::CoutN(r.m); }
                }, std::to_string((size_t)To<U>(L)));
                return 0;
            });
            SetFieldCClosure(L, "unschedule", [](auto L)->int { To<U>(L)->unschedule(std::to_string((size_t)To<U>(L))); return 0; });
            SetFieldCClosure(L, "unscheduleAllCallbacks", [](auto L)->int { To<U>(L)->unscheduleAllCallbacks(); return 0; });
            SetFieldCClosure(L, "runAction", [](auto L)->int { return Push(L, To<U>(L)->runAction(To<cocos2d::Action*>(L, 2))); });
            SetFieldCClosure(L, "stopAllActions", [](auto L)->int { To<U>(L)->stopAllActions(); return 0; });
            SetFieldCClosure(L, "stopAction", [](auto L)->int { To<U>(L)->stopAction(To<cocos2d::Action*>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopActionByTag", [](auto L)->int { To<U>(L)->stopActionByTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopAllActionsByTag", [](auto L)->int { To<U>(L)->stopAllActionsByTag(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "stopActionsByFlags", [](auto L)->int { To<U>(L)->stopActionsByFlags(To<int>(L, 2)); return 0; });
            SetFieldCClosure(L, "getActionByTag", [](auto L)->int { return Push(L, To<U>(L)->getActionByTag(To<int>(L, 2))); });
            SetFieldCClosure(L, "getNumberOfRunningActions", [](auto L)->int { return Push(L, To<U>(L)->getNumberOfRunningActions()); });
            SetFieldCClosure(L, "getEventDispatcher", [](auto L)->int { return Push(L, To<U>(L)->getEventDispatcher()); });
            // todo: getGLProgram setGLProgram getGLProgramState setGLProgramState ......


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
            SetFieldCClosure(L, "setTexture", [](auto L)->int { To<U>(L)->setTexture(To<std::string>(L, 2)); return 0; });
        }
    };

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
                return luaL_error(L, "'tostring' must return a string to 'print'");
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

    XL::SetGlobal(L, "Projection_2D", cocos2d::Director::Projection::_2D);
    XL::SetGlobal(L, "Projection_3D", cocos2d::Director::Projection::_3D);
    XL::SetGlobal(L, "Projection_CUSTOM", cocos2d::Director::Projection::CUSTOM);
    XL::SetGlobal(L, "Projection_DEFAULT", cocos2d::Director::Projection::DEFAULT);

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

    XL::SetGlobal(L, "ResolutionPolicy_EXACT_FIT", ResolutionPolicy::EXACT_FIT);
    XL::SetGlobal(L, "ResolutionPolicy_NO_BORDER", ResolutionPolicy::NO_BORDER);
    XL::SetGlobal(L, "ResolutionPolicy_SHOW_ALL", ResolutionPolicy::SHOW_ALL);
    XL::SetGlobal(L, "ResolutionPolicy_FIXED_HEIGHT", ResolutionPolicy::FIXED_HEIGHT);
    XL::SetGlobal(L, "ResolutionPolicy_FIXED_WIDTH", ResolutionPolicy::FIXED_WIDTH);
    XL::SetGlobal(L, "ResolutionPolicy_UNKNOWN", ResolutionPolicy::UNKNOWN);

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

    XL::SetGlobal(L, "EventListener_Type_UNKNOWN", cocos2d::EventListener::Type::UNKNOWN);
    XL::SetGlobal(L, "EventListener_Type_TOUCH_ONE_BY_ONE", cocos2d::EventListener::Type::TOUCH_ONE_BY_ONE);
    XL::SetGlobal(L, "EventListener_Type_TOUCH_ALL_AT_ONCE", cocos2d::EventListener::Type::TOUCH_ALL_AT_ONCE);
    XL::SetGlobal(L, "EventListener_Type_KEYBOARD", cocos2d::EventListener::Type::KEYBOARD);
    XL::SetGlobal(L, "EventListener_Type_MOUSE", cocos2d::EventListener::Type::MOUSE);
    XL::SetGlobal(L, "EventListener_Type_ACCELERATION", cocos2d::EventListener::Type::ACCELERATION);
    XL::SetGlobal(L, "EventListener_Type_FOCUS", cocos2d::EventListener::Type::FOCUS);
    XL::SetGlobal(L, "EventListener_Type_GAME_CONTROLLER", cocos2d::EventListener::Type::GAME_CONTROLLER);
    XL::SetGlobal(L, "EventListener_Type_CUSTOM", cocos2d::EventListener::Type::CUSTOM);

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
        return luaL_error(L, "cc_Animation_create error! need 2 ~ 3 args: SpriteFrame[] arrayOfAnimationFrameNames, float delayPerUnit, loops = 1");
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
        return luaL_error(L, "cc_Animation_createWithSpriteFrames error! need 1 ~ 3 args: SpriteFrame[] frames, float delay = 0, loop = 1");
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
        return XL::Push(L, cocos2d::CallFunc::create([f = XL::To<XL::Func>(L, 1)]{ f(); }));
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
            return luaL_error(L, "cc_MoveBy_create error! need 3 ~ 4 args: float duration, float deltaX, float deltaY, float deltaZ");
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
            return luaL_error(L, "cc_MoveTo_create error! need 3 ~ 4 args: float duration, float dstX, float dstY, float dstZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Node_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Node::create());
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
            return luaL_error(L, "cc_RotateTo_create error! need 2 ~ 4 args: float duration, float dstAngleX, float dstAngleY, float dstAngleZ");
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
            return luaL_error(L, "cc_RotateBy_create error! need 2 ~ 4 args: float duration, float dstAngleX, float dstAngleY, float dstAngleZ");
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
            return luaL_error(L, "cc_ScaleTo_create error! need 2 ~ 4 args: float duration, float sXY/sX, float sY, float sZ");
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
            return luaL_error(L, "cc_ScaleBy_create error! need 2 ~ 4 args: float duration, float sXY/sX, float sY, float sZ");
        }
    });

    XL::SetGlobalCClosure(L, "cc_Sequence_create", [](auto L) -> int {
        auto&& numArgs = lua_gettop(L);
        if (!numArgs) {
            return luaL_error(L, "; error! need 1+ args: FiniteTimeAction*... / { FiniteTimeAction*... }");
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
            return luaL_error(L, "cc_Spawn_create error! need 1+ args: FiniteTimeAction*... / { FiniteTimeAction*... }");
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




    //lua_pushstring(L, TypeNames<cocos2d::TextHAlignment>::value);
    //lua_createtable(L, 3, 0);
    //lua_pushstring(L, "LEFT");	lua_pushinteger(L, (int)cocos2d::TextHAlignment::LEFT);	lua_rawset(L, -3);
    //lua_pushstring(L, "CENTER");	lua_pushinteger(L, (int)cocos2d::TextHAlignment::CENTER);	lua_rawset(L, -3);
    //lua_pushstring(L, "RIGHT");	lua_pushinteger(L, (int)cocos2d::TextHAlignment::RIGHT);	lua_rawset(L, -3);
    //lua_rawset(L, -3);

    //lua_pushstring(L, TypeNames<cocos2d::TextVAlignment>::value);
    //lua_createtable(L, 3, 0);
    //lua_pushstring(L, "TOP");	lua_pushinteger(L, (int)cocos2d::TextVAlignment::TOP);	lua_rawset(L, -3);
    //lua_pushstring(L, "CENTER");	lua_pushinteger(L, (int)cocos2d::TextVAlignment::CENTER);	lua_rawset(L, -3);
    //lua_pushstring(L, "BOTTOM");	lua_pushinteger(L, (int)cocos2d::TextVAlignment::BOTTOM);	lua_rawset(L, -3);
    //lua_rawset(L, -3);






    // 创建场景
    XL::SetGlobalCClosure(L, "cc_Scene_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::create());
    });






    // 创建精灵
    XL::SetGlobalCClosure(L, "cc_Sprite_create", [](auto L) -> int {
        auto&& r = lua_gettop(L)
            ? cocos2d::Sprite::create(XL::To<std::string>(L, 1))
            : cocos2d::Sprite::create();
        return XL::Push(L, r);
    });

    // 注册帧回调函数
    XL::SetGlobalCClosure(L, "cc_setFrameUpdate", [](auto L) -> int {
        To(L, 1, _globalUpdate);
        return 0;
    });

    // 在 cocos 中注册 lua 帧回调
    _director->getScheduler()->schedule([](float delta) {
        if (_globalUpdate) {
            if (auto&& r = XL::Try(_luaState, [&] {
                    _globalUpdate(delta);
                })) {
                xx::CoutN(r.m);
            }
        }
    }, (void*)ad, 0, false, "AppDelegate");

    XL::AssertTop(L, 0);

    // 执行 main.lua
    if (auto&& r = XL::Try(L, [&] {
            xx::Lua::DoFile(L, "main.lua");
        })) {
        xx::CoutN(r.m);
    }
}
