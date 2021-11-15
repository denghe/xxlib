// 针对 adxe engine 的纯 cpp 导出项目的 lua 扩展
// 用法：
// 在 AppDelegate.cpp 第 34 行，也就是 一堆 include 之后，USING_NS_CC 之前 插入本 .h 的 include
// 并于 applicationDidFinishLaunching 的最后 执行 luaBinds(this);

#include <xx_lua_bind.h>
namespace XL = xx::Lua;

// 单例区
inline static AppDelegate* _appDelegate = nullptr;
inline static cocos2d::Director* _director = nullptr;
inline static lua_State* _luaState = nullptr;
inline static XL::Func _globalUpdate;  // 注意：restart 功能实现时需先 Reset

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
    // Node : Ref
    template<>
    struct MetaFuncs<cocos2d::Node*, void> {
        using U = cocos2d::Node*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
            SetFieldCClosure(L, "getPosition", [](auto L)->int { return Push(L, To<U>(L)->getPosition()); });
            SetFieldCClosure(L, "setPosition", [](auto L)->int { To<U>(L)->setPosition(To<cocos2d::Vec2>(L, 2)); return 0; });

            SetFieldCClosure(L, "getColor", [](auto L)->int { return Push(L, To<U>(L)->getColor()); });
            SetFieldCClosure(L, "setColor", [](auto L)->int { To<U>(L)->setColor(To<cocos2d::Color3B>(L, 2)); return 0; });

            SetFieldCClosure(L, "addChild", [](auto L)->int { To<U>(L)->addChild(To<cocos2d::Node*>(L, 2)); return 0; });
            SetFieldCClosure(L, "schedule", [](auto L)->int {
                To<U>(L)->schedule([f = To<Func>(L, 2)](float delta) {
                    if (auto r = Try(_luaState, [&] { f.Call(delta); })) { xx::CoutN(r.m); }
                }, std::to_string((size_t)To<U>(L)));
                return 0;
            });
            SetFieldCClosure(L, "getPositionXY", [](auto L)->int { auto&& o = To<U>(L)->getPosition(); return Push(L, o.x, o.y); });
            SetFieldCClosure(L, "setPositionXY", [](auto L)->int { To<U>(L)->setPosition(To<float>(L, 2), To<float>(L, 3)); return 0; });

            SetFieldCClosure(L, "getColorRGB", [](auto L)->int { auto&& o = To<U>(L)->getColor(); return Push(L, o.r, o.g, o.b); });
            SetFieldCClosure(L, "setColorRGB", [](auto L)->int { To<U>(L)->setColor({ To<uint8_t>(L, 2), To<uint8_t>(L, 3), To<uint8_t>(L, 4) }); return 0; });
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
            SetFieldCClosure(L, "setTexture", [](auto L)->int { To<U>(L)->setTexture(To<char const*>(L, 2)); return 0; });
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
            EnsureType<U>(L, idx);  // 只在 Debug 生效用 AssertType
            out = *(U*)lua_touserdata(L, idx);
        }
    };
}

void luaBinds(AppDelegate* ad) {
    // 创建环境
    _appDelegate = ad;
    _director = cocos2d::Director::getInstance();
    auto L = _luaState = luaL_newstate();
    luaL_openlibs(_luaState);

    // 简单给 lua 塞几个 cocos 函数

    // 创建场景
    XL::SetGlobalCClosure(L, "cc_scene_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::create());
        });

    // 加载场景
    XL::SetGlobalCClosure(L, "cc_director_runWithScene", [](auto L) -> int {
        auto&& scene = XL::To<cocos2d::Scene*>(L, 1);
        _director->runWithScene(scene);
        return 0;
        });

    // 创建精灵
    XL::SetGlobalCClosure(L, "cc_sprite_create", [](auto L) -> int {
        auto r = lua_gettop(L)
            ? cocos2d::Sprite::create(XL::To<char const*>(L, 1))
            : cocos2d::Sprite::create();
        return XL::Push(L, r);
        });

    // 注册帧回调函数
    XL::SetGlobalCClosure(L, "cc_setFrameUpdate", [](auto L) -> int {
        To(L, 1, _globalUpdate);
        return 0;
        });

    // 产生 lua 帧回调
    _director->getScheduler()->schedule([](float delta) {
        if (_globalUpdate) {
            if (auto r = XL::Try(_luaState, [&] {
                _globalUpdate.Call(delta);
                })) {
                xx::CoutN(r.m);
            }
        }
        }, (void*)ad, 0, false, "AppDelegate");

    XL::AssertTop(L, 0);

    // 模拟从文件读取 lua 源码( 具体要做 lua 文件操作还需要给 L 做映射，这里掠过 )
    auto luaSrc = R"(
local scene = cc_scene_create()
cc_director_runWithScene(scene)

local spr = cc_sprite_create()
spr:setTexture("HelloWorld.png")
spr:setPositionXY(300, 300)
scene:addChild(spr)

local c3 = spr:getColor()
c3[1] = 0
spr:setColor(c3)

local r, g, b = spr:getColorRGB()
g = 0;
spr:setColorRGB(r, g, b)

spr:schedule(function(delta)
    local v2 = spr:getPosition()
    v2[2] = v2[2] + delta * 20    -- change y
    spr:setPosition( v2 )
end)

cc_setFrameUpdate(function(delta)
    local x, y = spr:getPositionXY()
    spr:setPositionXY( x + delta * 20, y )    -- change x
end)

)";

    // 模拟执行 lua 源码
    auto r = XL::Try(L, [&] {
        xx::Lua::DoString(L, luaSrc);
        });
    if (r) {
        xx::CoutN(r.m);
    }
}
