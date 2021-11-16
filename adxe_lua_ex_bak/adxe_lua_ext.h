// 针对 adxe engine 的纯 cpp 导出项目的 lua 扩展
// 用法：
// 在 AppDelegate.cpp 第 34 行，也就是 一堆 include 之后，USING_NS_CC 之前 插入本 .h 的 include
// 并于 applicationDidFinishLaunching 的最后 执行 luaBinds(this);

#include <xx_lua_data.h>
namespace XL = xx::Lua;

// 单例区
inline static AppDelegate* _appDelegate = nullptr;
inline static cocos2d::FileUtils* _fileUtils = nullptr;
inline static cocos2d::Director* _director = nullptr;
inline static cocos2d::GLView* _glView = nullptr;   // lazy init
inline static cocos2d::ActionManager* _actionManager = nullptr;
inline static cocos2d::Scheduler* _scheduler = nullptr;
inline static cocos2d::EventDispatcher* _eventDispatcher = nullptr;
inline static lua_State* _luaState = nullptr;
inline static XL::Func _globalUpdate;   // 注意：restart 功能实现时需先 Reset
inline static std::string _string;      // for print
inline static std::vector<std::string> _strings;    // tmp

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
    // Action : Ref
    template<>
    struct MetaFuncs<cocos2d::Action*, void> {
        using U = cocos2d::Action*;
        inline static std::string name = std::string(TypeName_v<U>);
        static void Fill(lua_State* const& L) {
            MetaFuncs<cocos2d::Ref*>::Fill(L);
            SetType<U>(L);
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
                    if (auto&& r = Try(_luaState, [&] { f.Call(); })) { xx::CoutN(r.m); }
                });
                return 0;
            });
            SetFieldCClosure(L, "setOnExitCallback", [](auto L)->int {
                To<U>(L)->setOnExitCallback([f = To<Func>(L, 2)] {
                    if (auto&& r = Try(_luaState, [&] { f.Call(); })) { xx::CoutN(r.m); }
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
                    if (auto&& r = Try(_luaState, [&] { f.Call(delta); })) { xx::CoutN(r.m); }
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
    // xx::Data & FileUtils

    xx::Lua::Data::Register(L, "xx_data_create");

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
    // Node

    XL::SetGlobalCClosure(L, "cc_node_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Node::create());
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
    XL::SetGlobalCClosure(L, "cc_scene_create", [](auto L) -> int {
        return XL::Push(L, cocos2d::Scene::create());
    });






    // 创建精灵
    XL::SetGlobalCClosure(L, "cc_sprite_create", [](auto L) -> int {
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
                    _globalUpdate.Call(delta);
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
