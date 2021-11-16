// 针对 adxe engine 的纯 cpp 导出项目的 lua 扩展
// 用法：
// 在 AppDelegate.cpp 第 34 行，也就是 一堆 include 之后，USING_NS_CC 之前 插入本 .h 的 include
// 并于 applicationDidFinishLaunching 的最后 执行 luaBinds(this);

#include <xx_lua_bind.h>
namespace XL = xx::Lua;

// 单例区
inline static AppDelegate* _appDelegate = nullptr;
inline static cocos2d::FileUtils* _fileUtils = nullptr;
inline static cocos2d::Director* _director = nullptr;
inline static lua_State* _luaState = nullptr;
inline static XL::Func _globalUpdate;   // 注意：restart 功能实现时需先 Reset
inline static std::string _string;      // for print

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
    _fileUtils = cocos2d::FileUtils::getInstance();
    _director = cocos2d::Director::getInstance();
    auto L = _luaState = luaL_newstate();
    luaL_openlibs(_luaState);

    /******************************************************************/
    // 开始映射

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
    XL::SetGlobalCClosure(L, "lua_print", [](auto L) -> int {
        _string.clear();
        get_string_for_print(L, _string);
        CCLOG("[LUA-print] %s", _string.c_str());
        return 0;
    });

    // 翻写 cocos 的 lua_release_print
    XL::SetGlobalCClosure(L, "lua_release_print", [](auto L) -> int {
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

    // 在 cocos 中注册 lua 帧回调
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

    // 执行 main.lua
    if (auto r = XL::Try(L, [&] {
            xx::Lua::DoFile(L, "main.lua");
        })) {
        xx::CoutN(r.m);
    }
}
