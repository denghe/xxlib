#pragma once
#include "xx_lua.h"
#include "xx_ptr.h"

// lua 5.1 没有这个东西，需要自己动手，为每个 state 于创建后立刻执行下列语句以确保 LUA_RIDX_MAINTHREAD 的值跨 state 一致：
// auto rtv = lua_pushthread(L);
// assert(rtv == 1);    // must be main thread
// LUA_RIDX_MAINTHREAD = luaL_ref(L, LUA_REGISTRYINDEX);
#if LUA_VERSION_NUM == 501
extern int LUA_RIDX_MAINTHREAD;
#endif

namespace xx::Lua {

    /****************************************************************************************/
    /****************************************************************************************/
    // 将 指定 idx 的 luavalue 以 luaL_ref 方式放入 注册表
    // 通常由 c++ 结构体携带, 析构时可同步从 注册表 luaL_unref 删除
    struct RegistryStoreler {
        RegistryStoreler() = default;
        RegistryStoreler(RegistryStoreler const& o) = default;
        RegistryStoreler& operator=(RegistryStoreler const& o) = default;
        RegistryStoreler(RegistryStoreler&& o) = default;
        RegistryStoreler& operator=(RegistryStoreler&& o) = default;

        // 存储 L 和 ref 值
        Shared<std::pair<lua_State*, int>> p;

        RegistryStoreler(lua_State* const& L, int const& idx) {
            Reset(L, idx);
        }
        ~RegistryStoreler() {
            Reset();
        }

        // 从指定位置 pushvalue, 弄到注册表并存储返回的 ref 值
        void Reset(lua_State* const& L, int const& idx) {
            Reset();
            CheckStack(L, 2);
            lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);         // ..., tar, ..., L
            p.Emplace();
            p->first = lua_tothread(L, -1);
            lua_pop(L, 1);                                                  // ..., tar, ...
            lua_pushvalue(L, idx);											// ..., tar, ..., tar
            p->second = luaL_ref(L, LUA_REGISTRYINDEX);						// ..., tar, ...
        }

        // 如果 p 引用计数唯一, 则反注册
        void Reset() {
            if (p.GetSharedCount() == 1) {
                luaL_unref(p->first, LUA_REGISTRYINDEX, p->second);
            }
            p.Reset();
        }

        // 将 注册表 里的值 压到 栈
        void PushStore(lua_State* const& L) const {
            lua_rawgeti(L, LUA_REGISTRYINDEX, p->second);
        }

        // 判断是否有值
        operator bool() const {
            return (bool)p;
        }
    };

    // 用于指向一张存到 注册表 的 table
    struct TableRef : RegistryStoreler {
        using RegistryStoreler::RegistryStoreler;

        template<typename V>
        void Get(std::string_view const& k, V& v) const {
            assert(p);
            auto L = p->first;
            CheckStack(L, 4);
            PushStore(L);                                                   // ..., t
            GetField(L, -1, k, v);
            lua_pop(L, 1);                                                  // ...
        }
        template<typename V>
        V Get(std::string_view const& k) const {
            V v;
            Get(k, v);
            return v;
        }

        template<typename T>
        void Set(std::string_view key, T& val) {
            assert(p);
            auto L = p->first;
            CheckStack(L, 4);
            PushStore(L);                                                   // ..., t
            SetField(L, -1, key, val);
            lua_pop(L, 1);                                                  // ...
        }

        // todo: GetFields SetFields?
    };

    // 从 upvalue 中拿 userdata 转为目标类型的引用
    template<typename T>
    T& GetUpUD(lua_State* const& L, int idx = 1) {
        return *(T*)lua_touserdata(L, lua_upvalueindex(idx));
    }

    // 针对 含有 in->mt_ref 成员的类型, Push userdata + metatable ( 留在 stack 里 ), 并将 in->mt 指向该 metatable
    // 返回 in 在放入 userdata 之前的 原始指针值 备用( 避免 move 后 in 失效 )
    // 如果已存在 mt. 找回, 返回 nullptr
    template<typename T, typename E, TableRef E::* ptr>
    auto PushUdMt(lua_State* const& L, T&& in) {
        using U = std::decay_t<T>;
        auto o = &*in;
        auto ud = lua_newuserdata(L, sizeof(U));						// ..., ud
        new(ud) U(std::forward<T>(in));

        if (o->*ptr) {                                                  // 已存在 mt. 找回, 返回 nullptr
            (o->*ptr).PushStore(L);                                     // ..., ud, mt
            return (decltype(o))nullptr;
        }

        lua_createtable(L, 0, 20);                                      // ..., ud, mt
        (o->*ptr).Reset(L, -1);                                         //                                  store mt to registry

        lua_pushstring(L, "__gc");                                      // ..., ud, mt, "__gc"
        lua_pushcclosure(L, [](auto L)->int {                           // ..., ud, mt, "__gc", cc
            using UU = U;                                               // fix vs compile bug
            ((U*)lua_touserdata(L, -1))->~UU();
            return 0;
        }, 0);
        lua_rawset(L, -3);                                              // ..., ud, mt

        lua_pushstring(L, "__index");                                   // ..., ud, mt, "__index"
        lua_pushvalue(L, -2);                                           // ..., ud, mt, "__index", mt
        lua_rawset(L, -3);                                              // ..., ud, mt

        lua_pushstring(L, "__newindex");                                // ..., ud, mt, "__newindex"
        lua_pushcclosure(L, [](auto L)->int {                           // ..., ud, mt, "__newindex", cc
            lua_getmetatable(L, 1);                                         // sender, k, v, mt
            lua_replace(L, -4);                                             // mt, k, v
            lua_rawset(L, -3);                                              // mt
            return 0;
            }, 0);
        lua_rawset(L, -3);                                              // ..., ud, mt
        return o;
    }


	/****************************************************************************************/
	/****************************************************************************************/
	// lua 向 c++ 注册的回调函数的封装, 将函数以 luaL_ref 方式放入注册表
	// 通常被 lambda 捕获携带, 析构时同步删除 lua 端函数。 需确保比 L 早死, 否则 L 就野了
	struct Func : RegistryStoreler {
        using RegistryStoreler::RegistryStoreler;

		// 执行并返回 T, 最后还原堆栈
		template<typename T = void, typename...Args>
		T Call(Args&&...args) const {
			assert(p);
			auto& L = p->first;
			auto top = lua_gettop(L);
			CheckStack(L, 1);
            PushStore(L);					                                // ..., func
			auto n = Push(L, std::forward<Args>(args)...);					// ..., func, args...
			lua_call(L, n, LUA_MULTRET);									// ..., rtv...?
			if constexpr (!std::is_void_v<T>) {
				T rtv;
				To(L, top + 1, rtv);
				lua_settop(L, top);											// ...
				return rtv;
			}
			else {
				lua_settop(L, top);											// ...
			}
		}

        // 执行并用 lambda 处理返回值( 位于 lua_State, 自已搞 ), 最后还原堆栈
        // rh 通常传入 [&](auto L, int idx) { ... }
        template<typename RtvHandler, typename...Args>
        void ExecuteTo(RtvHandler&& rh, Args&&...args) const {
            assert(p);
            auto& L = p->first;
            auto top = lua_gettop(L);
            CheckStack(L, 1);
            PushStore(L);					                                // ..., func
            auto n = Push(L, std::forward<Args>(args)...);					// ..., func, args...
            lua_call(L, n, LUA_MULTRET);									// ..., rtv...?
            rh(L, top + 1);
            lua_settop(L, top);											    // ...
        }
	};

	// 适配 Func
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<Func, std::decay_t<T>>>> {
        static constexpr int checkStackSize = 1;
		static inline int Push_(lua_State* const& L, Func const& f) {
            if (f) {
                f.PushStore(L);
            } else {
                lua_pushnil(L);
            }
            return 1;
		}
		static inline void To_(lua_State* const& L, int const& idx, T& out) {
            if (!lua_isfunction(L, idx)) {
                out.Reset();
            }
            else {
                out.Reset(L, idx);
            }
		}
	};

	// helper
	template<typename K>
	inline Func GetGlobalFunc(lua_State* const& L, K const& k) {
		Func f;
		GetGlobal(L, k, f);
		return f;
	}

	// global[k] = cFunc( upvalues...
	template<typename K, typename...VS>
	inline void SetGlobalCClosure(lua_State* const& L, K&& k, lua_CFunction&& f, VS&&...upvalues) {
		CheckStack(L, 3 + sizeof...(VS));
		Push(L, std::forward<VS>(upvalues)...);			                    // ..., upvalues...
		lua_pushcclosure(L, f, sizeof...(VS));								// ..., cfunc
		if constexpr (std::is_base_of_v<std::string, std::decay_t<K>> || std::is_base_of_v<std::string_view, std::decay_t<K>>) {
			lua_setglobal(L, k.data());									// ...
		}
		else {
			lua_setglobal(L, k);											// ...
		}
	}

	// 栈顶 table[k] = cFunc( upvalues...
	template<typename K, typename...VS>
	inline void SetFieldCClosure(lua_State* const& L, K&& k, lua_CFunction&& f, VS&&...upvalues) {
		CheckStack(L, 3 + sizeof...(VS));
		Push(L, std::forward<K>(k), std::forward<VS>(upvalues)...);			// ..., table, k, upvalues...
		lua_pushcclosure(L, f, sizeof...(VS));								// ..., table, k, cfunc
		lua_rawset(L, -3);													// ..., table, 
	}


    /****************************************************************************************/
    /****************************************************************************************/
    // 类型 metatable 填充函数适配模板
    template<typename T, typename ENABLED = void>
    struct MetaFuncs {
        inline static std::string name = TypeName<T>();

        // 该函数被调用时, 栈顶即为 mt
        // 使用 SetType<std::decay_t<T>>(L); 附加 type 信息
        // 调用 MetaFuncs<基类>::Fill(L); 填充依赖
        // 使用 SetFieldCClosure(L, "key", [](auto L) { ..... return ?; }, ...) 添加函数
        // 使用 luaL_setfuncs 批量添加函数也行
        static void Fill(lua_State* const& L) {}
    };


    // 判断指定 idx 所在是指定类型的 userdata ( 判断带 mt && mt[Typename*] == mt )
	template<typename T>
	inline bool IsUserdata(lua_State* const& L, int const& idx) {
		CheckStack(L, 2);
		if (!lua_isuserdata(L, idx)) return false;
		lua_getmetatable(L, idx);											// ..., mt?
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return false;
		}
		Push(L, (void*)MetaFuncs<T>::name.data());							// ..., mt, k
		lua_rawget(L, -2);													// ..., mt, mt?
		bool rtv = lua_rawequal(L, -1, -2);
		lua_pop(L, 2);
		return rtv;
	}

	// 为当前栈顶的 mt 附加 type 信息
	template<typename T>
	inline void SetType(lua_State* const& L) {
		CheckStack(L, 3);
		Push(L, (void*)MetaFuncs<T>::name.data());							// ..., mt, k
		lua_pushvalue(L, -2);												// ..., mt, k, mt
		lua_rawset(L, -3);													// ..., mt
	}

	template<typename T>
	inline void EnsureType(lua_State* const& L, int const& idx) {
		if (!IsUserdata<T>(L, idx)) {
			Error(L, "error! args[", idx, "] is not ", MetaFuncs<T>::name);
		}
	}

	template<typename T>
	inline void AssertType(lua_State* const& L, int const& idx) {
#ifndef NDEBUG
		EnsureType<T>(L, idx);
#endif
	}


	// 压入指定类型的 metatable( 以 MetaFuncs<T>::name.data() 为 key, 存到注册表。没有找到就创建并放入 )
	template<typename T, bool useCustomGC = false>
	void PushMeta(lua_State* const& L) {
		CheckStack(L, 3);
#if LUA_VERSION_NUM == 501
		lua_pushlightuserdata(L, (void*)MetaFuncs<T>::name.data());         // ..., key
		lua_rawget(L, LUA_REGISTRYINDEX);                                   // ..., mt?
#else
		lua_rawgetp(L, LUA_REGISTRYINDEX, MetaFuncs<T>::name.data());       // ..., mt?
#endif
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);                                                  // ...
			CheckStack(L, 4);
			lua_createtable(L, 0, 20);                                      // ..., mt

			// 如果类型需要析构就自动生成 __gc
			if constexpr (!useCustomGC && std::is_destructible_v<T>) {
				lua_pushstring(L, "__gc");                                  // ..., mt, "__gc"
				lua_pushcclosure(L, [](auto L)->int {                       // ..., mt, "__gc", cc
					using U = T;	// fix vs compile bug
					auto f = (U*)lua_touserdata(L, -1);
					f->~U();
					return 0;
					}, 0);
				lua_rawset(L, -3);                                          // ..., mt
			}

			// 如果压入的不是 lambda 或 std::function 这种仿函数类, 则继续填充各种适配
			if constexpr (!IsLambda_v<T> && !IsFunction_v<T>) {
				lua_pushstring(L, "__index");                               // ..., mt, "__index"
				lua_pushvalue(L, -2);                                       // ..., mt, "__index", mt
				lua_rawset(L, -3);                                          // ..., mt

			    MetaFuncs<T, void>::Fill(L);                                // ..., mt

#if LUA_VERSION_NUM == 501
			    lua_pushlightuserdata(L, (void*)MetaFuncs<T>::name.data()); // ..., mt, key
			    lua_pushvalue(L, -2);                                       // ..., mt, key, mt
			    lua_rawset(L, LUA_REGISTRYINDEX);                           // ..., mt
#else
			    lua_pushvalue(L, -1);                                       // ..., mt, mt
			    lua_rawsetp(L, LUA_REGISTRYINDEX, MetaFuncs<T>::name.data());   // ..., mt
#endif
			    //CoutN("PushMeta MetaFuncs<T>::name.data() = ", (size_t)MetaFuncs<T>::name.data(), " T = ", MetaFuncs<T>::name);
			}
		}
	}


	// 在 userdata 申请 T 的内存 并执行构造函数, 传入 args 参数。最后附加 mt
	template<typename T, bool useCustomGC = false, typename ...Args>
	int PushUserdata(lua_State* const& L, Args&&...args) {
		CheckStack(L, 2);
		auto p = lua_newuserdata(L, sizeof(T));								// ..., ud
		new(p) T(std::forward<Args>(args)...);
		PushMeta<T, useCustomGC>(L);										// ..., ud, mt
		lua_setmetatable(L, -2);											// ..., ud
		return 1;
	}


    // 适配 lambda 等可执行类 ( 这个性能一般，不要滥用，尽量使用 CClosure )
    // 在 userdata 申请 lambda 捕获上下文的内存 并将 lambda 挪进去, 参数利用 tuple 暂存并展开转发。附加的 mt 啥都没有，只有 __gc 析构
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<IsLambda_v<T> || IsFunction_v<T>>> {
        static constexpr int checkStackSize = 1;
        static inline int Push_(lua_State* const& L, T&& in) {
            using U = std::decay_t<T>;
            PushUserdata<U>(L, std::forward<T>(in));						// ..., ud
            lua_pushcclosure(L, [](auto L) {								// ..., cc
                auto f = (U*)lua_touserdata(L, lua_upvalueindex(1));
                FuncA2_t<U> tuple;
                To(L, 1, tuple);
                int rtv = 0;
                std::apply([&](auto &&... args) {
                    if constexpr (std::is_void_v<FuncR_t<U>>) {
                        (*f)(args...);
                    }
                    else {
                        rtv = Push(L, (*f)(args...));
                    }
                    }, tuple);
                return rtv;
                }, 1);
            return 1;
        }
    };


    /****************************************************************************************/
    /****************************************************************************************/
    // 为支持 lua 中的一些简单工具类( 例如 Random? ) 的序列化 而设计一套映射体系
    // 这种 userdata 的 mt 需携带 UserdataId, 以方便调用相应的 读取函数

    enum class ValueTypes : uint8_t {
        NilType, True, False, Integer, Double, String, Userdata, Table, TableEnd
    };

    // userdata -- id 映射
    template<typename T>
    struct UserdataId {
        static const uint16_t value = 0;
    };

    template<typename T>
    constexpr uint16_t UserdataId_v = UserdataId<T>::value;

    // 这些 userdata 的 mt 需要以 (void*)Key_UserdataId.data() 为 key
    inline static std::string Key_UserdataId("UserdataId");

    // 给栈顶 mt 设置 UserdataId key value
    template<typename T>
    void SetUserdataId(lua_State* const& L) {
        static_assert(UserdataId_v<T> != 0);
        SetField(L, (void*)Key_UserdataId.data(), UserdataId_v<T>);         // ..., mt
    }

    // 从指定 idx 的 userdata 读出 UserdataId. 返回 0 表示出错
    inline uint16_t GetUserdataId(lua_State* const& L, int const& idx = -1) {
        CheckStack(L, 2);
        lua_getmetatable(L, idx);											// ... ud ..., mt
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return 0;
        }
        auto uid = GetField<uint16_t>(L, idx, (void*)Key_UserdataId.data());
        lua_pop(L, 1);                                                      // ... ud ...
        return uid;
    }

    // id 对应的 ReadFrom 函数( 读取 data 并在 L 顶创建相应的 userdata )
    typedef int(*UserdataReadFromFunc)(lua_State* const& L, Data_r& d);

    // Userdata ReadFrom 函数集
    inline static std::array<UserdataReadFromFunc, 65536> urffs{};

    // 适配范例:
    // urffs[UserdataId_v<TTTTTTT>] = [](lua_State* const& L, Data_r& d)->int { ...... }

    // 根据 id 调用相应 ReadFrom
    inline int UserdataReadFrom(lua_State* const& L, Data_r& d, uint16_t const& uid) {
        assert(urffs[uid]);
        return urffs[uid](L, d);
    }



    // id 对应的 WriteTo 函数( 将 L 中位于 index 位置的 userdata 写到 data )
    typedef void(*UserdataWriteToFunc)(lua_State* const& L, Data& d, int const& idx);

    // Userdata WriteTo 函数集
    inline static std::array<UserdataWriteToFunc, 65536> uwtfs{};

    // 适配范例:
    // uwtfs[UserdataId_v<TTTTTTT>] = [](lua_State* const& L, Data& d, int const& idx)->void { ...... }


    // 将栈顶的数据写入 Data (不支持 table 循环引用, 注意： lua5.1 不支持 int64, 这里并没有走 userdata 规则)
    static inline void WriteTo(Data& d, lua_State* const& L) {
        switch (auto t = lua_type(L, -1)) {
            case LUA_TNIL:
                d.WriteFixed(ValueTypes::NilType);
                return;
            case LUA_TBOOLEAN:
                d.WriteFixed(lua_toboolean(L, -1) ? ValueTypes::True : ValueTypes::False);
                return;
            case LUA_TNUMBER: {
#if LUA_VERSION_NUM == 501
                auto n = (double)lua_tonumber(L, -1);
            auto i = (int64_t)n;
            if ((double)i == n) {
                d.WriteFixed(ValueTypes::Integer);
                d.WriteVarInteger(i);
            }
            else {
                d.WriteFixed(ValueTypes::Double);
                d.WriteFixed(n);
            }
#else
                if (lua_isinteger(L, -1)) {
                    d.WriteFixed(ValueTypes::Integer);
                    d.WriteVarInteger((int64_t)lua_tointeger(L, -1));
                }
                else {
                    d.WriteFixed(ValueTypes::Double);
                    d.WriteFixed((double)lua_tonumber(L, -1));
                }
#endif
                return;
            }
            case LUA_TSTRING: {
                size_t len;
                auto ptr = lua_tolstring(L, -1, &len);
                d.WriteFixed(ValueTypes::String);
                d.WriteVarInteger(len);
                d.WriteBuf(ptr, len);
                return;
            }
            case LUA_TTABLE: {
                d.WriteFixed(ValueTypes::Table);
                int idx = lua_gettop(L);                                   // 存 idx 备用
                CheckStack(L, 1);
                lua_pushnil(L);                                            //                      ... t, nil
                while (lua_next(L, idx) != 0) {                            //                      ... t, k, v
                    // 先检查下 k, v 是否为不可序列化类型. 如果是就 next
                    t = lua_type(L, -1);
                    if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TTHREAD) {
                        lua_pop(L, 1);                                     //                      ... t, k
                        continue;
                    }
                    t = lua_type(L, -2);
                    if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TTHREAD) {
                        lua_pop(L, 1);                                     //                      ... t, k
                        continue;
                    }
                    WriteTo(d, L);                                         // 先写 v               ... t, k, v
                    lua_pop(L, 1);                                         //                      ... t, k
                    WriteTo(d, L);                                         // 再写 k
                }
                d.WriteFixed(ValueTypes::TableEnd);
                return;
            }
            case LUA_TUSERDATA: {
                if (auto uid = GetUserdataId(L)) {
                    d.WriteFixed(ValueTypes::Userdata);
                    d.WriteFixed(uid);
                    uwtfs[uid](L, d, -1);
                }
                else {
                    d.WriteFixed(ValueTypes::NilType);
                }
            }
            default:
                d.WriteFixed(ValueTypes::NilType);
                return;
        }
    }

    // 从 Data 读出一个 lua value 压入 L 栈顶
    // 如果读失败, 可能会有残留数据已经压入，外界需自己做 lua state 的 cleanup
    static inline int ReadFrom(Data_r& d, lua_State* const& L) {
        ValueTypes lt;
        if (int r = d.ReadFixed(lt)) return r;
        switch (lt) {
            case ValueTypes::NilType:
                CheckStack(L, 1);
                lua_pushnil(L);
                return 0;
            case ValueTypes::True:
                CheckStack(L, 1);
                lua_pushboolean(L, 1);
                return 0;
            case ValueTypes::False:
                CheckStack(L, 1);
                lua_pushboolean(L, 0);
                return 0;
            case ValueTypes::Integer: {
                int64_t i;
                if (int r = d.ReadVarInteger(i)) return r;
                CheckStack(L, 1);
                lua_pushinteger(L, (lua_Integer)i);
                return 0;
            }
            case ValueTypes::Double: {
                lua_Number v;
                if (int r = d.ReadFixed(v)) return r;
                CheckStack(L, 1);
                lua_pushnumber(L, v);
                return 0;
            }
            case ValueTypes::String: {
                size_t len;
                if (int r = d.ReadVarInteger(len)) return r;
                if (d.offset + len >= d.len) return __LINE__;
                CheckStack(L, 1);
                lua_pushlstring(L, (char*)d.buf + d.offset, len);
                d.offset += len;
                return 0;
            }
            case ValueTypes::Userdata: {
                uint16_t uid;
                if (int r = d.ReadFixed(uid)) return r;
                assert(urffs[uid]);
                return urffs[uid](L, d);
            }
            case ValueTypes::Table: {
                CheckStack(L, 4);
                lua_newtable(L);                                          // ... t
                while (d.offset < d.len) {
                    if (d.buf[d.offset] == (char)ValueTypes::TableEnd) {
                        ++d.offset;
                        return 0;
                    }
                    if (int r = ReadFrom(d, L)) return r;                 // ... t, v
                    if (int r = ReadFrom(d, L)) return r;                 // ... t, v, k
                    lua_pushvalue(L, -2);                                 // ... t, v, k, v
                    lua_rawset(L, -4);                                    // ... t, v
                    lua_pop(L, 1);                                        // ... t
                }
            }
            default:
                return __LINE__;
        }
    }

    // 注册一些常用的全局函数，变量
    inline void RegisterBaseEnv(lua_State* const& L) {

        SetGlobal(L, "null", (void*)0);
        SetGlobal(L, "NULL", (void*)0);
        SetGlobal(L, "nullptr", (void*)0);

        SetGlobalCClosure(L, "NowEpochSeconds", [](auto L)->int {
            return Push(L, NowSteadyEpochSeconds());
        });

        SetGlobalCClosure(L, "NowEpochMS", [](auto L)->int {
            return Push(L, NowSteadyEpochMilliseconds());
        });

        SetGlobalCClosure(L, "NowEpoch10m", [](auto L)->int { 
            return Push(L, NowSteadyEpoch10m());
        });


        SetGlobalCClosure(L, "Epoch10mToDateTime", [](auto L)->int {
            auto e10m = To<int64_t>(L, 1);
            auto tp = Epoch10mToTimePoint(e10m);
            auto t = std::chrono::system_clock::to_time_t(tp);
            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif
            return Push(L
                , tm.tm_year + 1900
                , tm.tm_mon + 1
                , tm.tm_mday
                , tm.tm_hour
                , tm.tm_min
                , tm.tm_sec
                , tm.tm_wday
                , tm.tm_yday);
        });

        // todo: more
    }
}
