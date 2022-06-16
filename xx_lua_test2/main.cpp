﻿#include "xx_lua_bind.h"
#include "xx_string.h"
namespace XL = xx::Lua;

namespace xx::Lua {
    struct MT : RegistryStoreler {
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
}

struct Foo {
    int id = 123;
    xx::Lua::MT mt;   // 压入 lua 后，该值指向 private metatable
};
typedef std::shared_ptr<Foo> Foo_s;

template<typename T>
T& GetUpUD(lua_State* const& L, int idx = 1) {
    return *(T*)lua_touserdata(L, lua_upvalueindex(idx));
}

namespace xx::Lua {
    template<typename T>
    struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Foo_s>>> {
        using U = Foo_s;
        using V = U::element_type;
        static int Push(lua_State* const& L, T&& in) {
            CheckStack(L, 4);
            if (!in) {
                lua_pushnil(L);
                return 1;
            }
            auto o = &*in;
            auto ud = lua_newuserdata(L, sizeof(U));						// ..., ud
            new(ud) U(std::forward<T>(in));                                 //                                  这之后 in 不可再用, 用 o, p

            lua_createtable(L, 0, 20);                                      // ..., ud, mt
            o->mt.Reset(L, -1);                                             //                                  store mt to registry

            lua_pushstring(L, "__gc");                                      // ..., ud, mt, "__gc"
            lua_pushcclosure(L, [](auto L)->int {                           // ..., ud, mt, "__gc", cc
                ((U*)lua_touserdata(L, -1))->~U();
                return 0;
            }, 0);
            lua_rawset(L, -3);                                              // ..., ud, mt

            lua_pushstring(L, "__index");                                   // ..., ud, mt, "__index"
            lua_pushvalue(L, -2);                                           // ..., ud, mt, "__index", mt
            lua_rawset(L, -3);                                              // ..., ud, mt

            lua_pushstring(L, "__newindex");                                // ..., ud, mt, "__newindex"
            lua_pushcclosure(L, [](auto L)->int {                           // ..., ud, mt, "__newindex", cc
                lua_getmetatable(L, 1);                                         // sender, k, v, mt
                lua_pushvalue(L, -3);                                           // sender, k, v, mt, k
                lua_pushvalue(L, -3);                                           // sender, k, v, mt, k, v
                lua_rawset(L, -3);                                              // sender, k, v, mt
                lua_pop(L, 1);                                                  // sender, k, v
                return 0;
            }, 0);
            lua_rawset(L, -3);                                              // ..., ud, mt

            auto p = (void*)o;
            SetFieldCClosure(L, "get_id", [](auto L)->int { return xx::Lua::Push(L, GetUpUD<V>(L).id); }, p);
            SetFieldCClosure(L, "set_id", [](auto L)->int { xx::Lua::To(L, 1, GetUpUD<V>(L).id); return 0; }, p);

            lua_setmetatable(L, -2);										// ..., ud
            return 1;
        }

        static void To(lua_State* const& L, int const& idx, U& out) {
            out = *(U*)lua_touserdata(L, idx);
        }
    };
}

int main() {
    XL::State L;
    XL::SetGlobalCClosure(L, "newFoo", [](auto L)->int {
        return XL::Push(L, std::make_shared<Foo>());
    });
    if (auto r = XL::Try(L, [&] {
        XL::DoString(L, R"(
foo = newFoo()
print(foo)
foo.set_id(12345)
print(foo.get_id())
foo.abc = 23423423
)");
        auto foo = XL::GetGlobal<Foo_s>(L, "foo");
        xx::CoutN("foo.abc = ", foo->mt.Get<double>("abc"));
    })) {
        xx::CoutN("catch error n = ", r.n, " m = ", r.m);
    }
    return 0;
}






//#include "xx_lua_bind.h"
//#include "xx_string.h"
//namespace XL = xx::Lua;
//
//template<typename T>
//T& GetThisFromTable(lua_State* L, int idx = 1) {    // ..., t, ...
//    lua_pushstring(L, "this");                      // ..., t, ..., "this"
//    lua_rawget(L, idx);                             // ..., t, ..., mem
//    auto& r = *(T*)lua_touserdata(L, -1);
//    lua_pop(L, 1);                                  // ..., t, ...
//    return r;
//}
//
//template<typename T>
//int PushThisToTable(lua_State* L, T&& v) {
//    using U = std::decay_t<T>;
//    lua_newtable(L);                                // ..., t
//    lua_pushstring(L, "this");                      // ..., t, "this"
//    auto ptr = lua_newuserdata(L, sizeof(T));       // ..., t, "this", mem
//    new (ptr) T(std::forward<T>(v));                // ..., t, "this", v
//
//    lua_newtable(L);                                // ..., t, "this", v, mt
//    lua_pushstring(L, "__gc");                      // ..., t, "this", v, mt, "__gc"
//    lua_pushcclosure(L, [](auto L)->int{            // ..., t, "this", v, mt, "__gc", lambda
//        auto ptr = lua_touserdata(L, 1);
//        ((U*)ptr)->~U();
//        return 0;
//    }, 0);
//    lua_settable(L, -3);                            // ..., t, "this", v, mt
//    lua_setmetatable(L, -2);                        // ..., t, "this", v
//    lua_settable(L, -3);                            // ..., t
//    return 1;
//}
//
//struct Foo {
//    int id = 123;
//    xx::Lua::Func cb;   // 指向 lua 返回 table 的函数
//};
//typedef std::shared_ptr<Foo> Foo_s;
//
//namespace xx::Lua {
//    template<typename T>
//    struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Foo_s>>> {
//        using U = Foo_s;
//        static int Push(lua_State* const& L, T&& in) {
//            auto r = PushThisToTable(L, std::forward<T>(in));
//            SetFieldCClosure(L, "get_id", [](auto L)->int {
//                auto& self = GetThisFromTable<U>(L);
//                return xx::Lua::Push(L, self->id);
//            });
//            SetFieldCClosure(L, "set_id", [](auto L)->int{
//                auto& self = GetThisFromTable<U>(L);
//                self->id = xx::Lua::To<int>(L, 2);
//                return 0;
//            });
//            SetFieldCClosure(L, "set_cb", [](auto L)->int{
//                auto& self = GetThisFromTable<U>(L);
//                self->cb = xx::Lua::To<Func>(L, 2);
//                return 0;
//            });
//            return r;
//        }
//        static void To(lua_State* const& L, int const& idx, U& out) {
//            out = GetThisFromTable<U>(L, idx);
//        }
//    };
//}
//
//struct TableFieldReader {
//    std::string key;
//    double value;
//};
//namespace xx::Lua {
//    template<typename T>
//    struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, TableFieldReader>>> {
//        using U = TableFieldReader;
//        static void To(lua_State* const& L, int const& idx, U& out) {
//            GetField(L, idx, out.key, out.value);
//        }
//    };
//}
//
//int main() {
//    XL::State L;
//    lua_newtable(L);
//    XL::SetFieldCClosure(L, "new", [](auto L)->int {
//        return XL::Push(L, std::make_shared<Foo>());
//    });
//    lua_setglobal(L, "Foo");
//    XL::DoString(L, R"(
//local Foo_new = Foo.new
//Foo.new = function()
//    local foo = Foo_new()
//    foo:set_cb( function() return foo end )
//    return foo
//end
//
//foo = Foo.new()
//print(foo)
//print(foo:get_id())
//foo.abc = 23423423
//)");
//    auto foo = XL::GetGlobal<Foo_s>(L, "foo");
//    TableFieldReader tfr { "abc", 0 };
//    foo->cb.ExecuteTo(tfr);
//    std::cout << tfr.value << std::endl;
//
//    return 0;
//}




//// 测试 lua 调用损耗
//#include "xx_lua_bind.h"
//#include "xx_string.h"
//
//struct Foo {
//    int id = 123;
//    xx::Lua::Func luafunc;
//};
//
//namespace XL = xx::Lua;
//int main() {
//    XL::State L;
//    if (auto r = XL::Try(L, [&]{
//        XL::DoString(L, R"(
//counter = 0
//
//function test_counter_get()
//    return counter
//end
//
//function test_counter_inc()
//    counter = counter + 1
//end
//
//function test_sum_n_times(n)
//    local f = sum
//    for i = 1, n, 1 do
//        f(i, i)
//    end
//end
//)");
//        XL::SetGlobalCClosure(L, "sum", [](lua_State* L)->int {
//            return XL::Push(L, XL::To<int>(L, 1) + XL::To<int>(L, 2));
//        });
//        auto test_sum_n_times = XL::GetGlobalFunc(L, "test_sum_n_times");
//        auto secs = xx::NowEpochSeconds();
//        test_sum_n_times(100000000);
//        std::cout << "call test_sum_n_times(100000000)  elapsed secs = " << xx::NowEpochSeconds(secs) << std::endl;
//
//        auto fg = XL::GetGlobalFunc(L, "test_counter_get");
//        auto fi = XL::GetGlobalFunc(L, "test_counter_inc");
//        secs = xx::NowEpochSeconds();
//        for (size_t i = 0; i < 100000000; i++) {
//            fi();
//        }
//        std::cout << "for 100000000 test_counter_inc  elapsed secs = " << xx::NowEpochSeconds(secs) << std::endl;
//        auto rr = fg.Call<int>();
//        std::cout << "counter = " << rr << std::endl;
//    })) {
//        xx::CoutN("catch error n = ", r.n, " m = ", r.m);
//    }
//    return 0;
//}
//






//// 测试结论：返回 self 实现连 : 写法，比不返回，要慢 1/10 左右. 成员函数（走元表）和缓存到local 的性能差异，大约 1/10。
//#include "xx_lua_bind.h"
//namespace XL = xx::Lua;
//struct Foo {
//    float counter = 0;
//    void xxxx(float x, float y) {
//        counter += x;
//        counter += y;
//    }
//};
//namespace xx::Lua {
//    template<>
//    struct MetaFuncs<std::shared_ptr<Foo>, void> {
//        using U = std::shared_ptr<Foo>;
//        inline static std::string name = std::string(TypeName_v<U>);
//        static void Fill(lua_State* const& L) {
//            SetType<U>(L);
//            SetFieldCClosure(L, "xxxx", [](auto L)->int {
//                To<U>(L)->xxxx(To<float>(L, 2), To<float>(L, 3));
//                return 0;
//            });
//            SetFieldCClosure(L, "yyyy", [](auto L)->int {
//                To<U>(L)->xxxx(To<float>(L, 2), To<float>(L, 3));
//                lua_settop(L, 1);
//                return 1;
//            });
//            SetFieldCClosure(L, "counter", [](auto L)->int {
//                return Push(L, To<U>(L)->counter);
//            });
//        }
//    };
//    template<typename T>
//    struct PushToFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::shared_ptr<Foo>>>> {
//        using U = std::decay_t<T>;
//        static int Push(lua_State* const& L, T&& in) {
//            return PushUserdata<U>(L, in);
//        }
//        static void To(lua_State* const& L, int const& idx, T& out) {
//#if XX_LUA_TO_ENABLE_TYPE_CHECK
//            EnsureType<U>(L, idx);  // 在 Release 也生效
//#endif
//            out = *(U*)lua_touserdata(L, idx);
//        }
//    };
//}
//int main() {
//    XL::State L;
//    XL::SetGlobalCClosure(L, "create_Foo", [](auto L){
//        return XL::Push(L, std::make_shared<Foo>());
//    });
//    for (int i = 0; i < 10; ++i) {
//        auto secs = xx::NowEpochSeconds();
//        XL::DoString(L, R"(
//local foo = create_Foo()
//local f = foo.xxxx
//for i = 1, 10000000 do
//    f( foo, 12, 34 )
//end
//print(foo:counter())
//)");
//        xx::CoutN("f( foo, 12, 34 ) secs = ", xx::NowEpochSeconds(secs));
//        XL::DoString(L, R"(
//local foo = create_Foo()
//local f = foo.xxxx
//local t = {}
//t.xxxx = function(a, b) f(foo, a, b) end
//for i = 1, 10000000 do
//    t.xxxx( 12, 34 )
//end
//print(foo:counter())
//)");
//        xx::CoutN("t.xxxx( 12, 34 ) secs = ", xx::NowEpochSeconds(secs));
//        XL::DoString(L, R"(
//local foo = create_Foo()
//local f = foo.xxxx
//local ff = function(a, b) f(foo, a, b) end
//for i = 1, 10000000 do
//    ff( 12, 34 )
//end
//print(foo:counter())
//)");
//        xx::CoutN("ff( 12, 34 ) secs = ", xx::NowEpochSeconds(secs));
//        XL::DoString(L, R"(
//local foo = create_Foo()
//for i = 1,10000000 do
//    foo:xxxx( 12, 34 )
//end
//print(foo:counter())
//)");
//        xx::CoutN("foo:xxxx( 12, 34 ) secs = ", xx::NowEpochSeconds(secs));
////        XL::DoString(L, R"(
////local foo = create_Foo()
////for i = 1,10000000 do
////    foo:yyyy( foo, 12, 34 ):yyyy( 12, 34 ):yyyy( 12, 34 )
////end
////print(foo:counter())
////)");
////        xx::CoutN("foo::yyyy call 3 times secs = ", xx::NowEpochSeconds(secs));
//    }
//    return 0;
//}




// 这是测试 lua cpp 编译模式的异常。感觉没啥问题。
//#include "xx_lua_bind.h"
//#include "xx_string.h"
//
//namespace XL = xx::Lua;
//int main() {
//    XL::State L;
//    XL::SetGlobal(L, "asdf", [&]{
//        auto s = "asdf";
//        [[maybe_unused]] auto sg = xx::MakeScopeGuard([&]{
//            xx::CoutN(s);
//        });
//        throw std::runtime_error("this is std runtime error");
//    });
//    if (auto r = XL::Try(L, [&]{
//        XL::DoString(L, "asdf()");
//    })) {
//        xx::CoutN("catch error n = ", r.n, " m = ", r.m);
//    }
//    return 0;
//}




//#include <iostream>
//#include "xx_lua_bind.h"
//#include "xx_lua_data.h"
//#include "xx_string.h"
//
//
//// Test result: native performance is pretty good.
//// There is no performance improvement in the memory pool written by myself,
//// and the direct link to mimalloc has greatly improved
//
//namespace xx {
///*
//    lua sample:
//
//    xx::MemPool lmp;
//    ...
//    auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize) {
//        return ((xx::MemPool*)ud)->Realloc(ptr, nsize, osize);
//    }, &lmp);
//*/
//    struct MemPool {
//        static_assert(sizeof(size_t) == sizeof(void *));
//        std::array<void *, sizeof(void *) * 8> blocks{};
//
//
//        ~MemPool() {
//            for (auto b: blocks) {
//                while (b) {
//                    auto next = *(void **) b;
//                    free(b);
//                    b = next;
//                }
//            }
//        }
//
//        void *Alloc(size_t siz) {
//            assert(siz);
//            // reserve header space
//            siz += sizeof(void *);
//
//            // align with 2^n
//            auto idx = Calc2n(siz);
//            if (siz > (size_t(1) << idx)) {
//                siz = size_t(1) << ++idx;
//            }
//
//            // try get pointer from chain
//            auto p = blocks[idx];
//            if (p) {
//                // store next pointer at header space
//                blocks[idx] = *(void **) p;
//            } else {
//                p = malloc(siz);
//            }
//
//            // error: no more free memory?
//            if (!p) return nullptr;
//
//            // store idx at header space
//            *(size_t *) p = idx;
//            return (void **) p + 1;
//        }
//
//        void Free(void *const &p) {
//            if (!p) return;
//
//            // calc header
//            auto h = (void **) p - 1;
//
//            // calc blocks[ index ]
//            auto idx = *(size_t *) h;
//
//            // store next pointer at header space
//            *(void **) h = blocks[idx];
//
//            // header link
//            blocks[idx] = h;
//        }
//
//        void *Realloc(void *p, size_t const &newSize, size_t const &dataLen = -1) {
//            // free only
//            if (!newSize) {
//                Free(p);
//                return nullptr;
//            }
//            // alloc only
//            if (!p) return Alloc(newSize);
//
//            // begin realloc
//
//            // free part1
//            auto h = (void **) p - 1;
//            auto idx = *(size_t *) h;
//
//            // calc original cap
//            auto cap = (size_t) ((size_t(1) << idx) - sizeof(void *));
//            if (cap >= newSize) return p;
//
//            // new & copy data
//            auto np = Alloc(newSize);
//            memcpy(np, p, std::min(cap, dataLen));
//
//            // free part2
//            *(void **) h = blocks[idx];
//            blocks[idx] = h;
//
//            return np;
//        }
//    };
//
//
//    struct MemPool2 {
//        static_assert(sizeof(size_t) == sizeof(void *));
//        std::array<std::vector<void *>, sizeof(void *) * 8> blocks{};
//
//        ~MemPool2() {
//            for (auto &bs: blocks) {
//                for (auto &b: bs) {
//                    free((size_t*)b - 1);
//                }
//            }
//        }
//
//        void *Alloc(size_t siz) {
//            assert(siz);
//            // reserve header space
//            siz += sizeof(size_t);
//
//            // align with 2^n
//            auto idx = Calc2n(siz);
//            if (siz > (size_t(1) << idx)) {
//                siz = size_t(1) << ++idx;
//            }
//
//            // try get or make
//            auto &bs = blocks[idx];
//            if (!bs.empty()) {
//                auto r = bs.back();
//                bs.pop_back();
//                return r;
//            } else {
//                auto p = (size_t *)malloc(siz);
//                if (!p) return nullptr;
//                *p = idx;
//                return p + 1;
//            }
//        }
//
//        void Free(void *const &p) {
//            if (!p) return;
//            auto idx = *((size_t*)p - 1);
//            blocks[idx].push_back(p);
//        }
//
//        void *Realloc(void *p, size_t const &newSize, size_t const &dataLen = -1) {
//            // free only
//            if (!newSize) {
//                Free(p);
//                return nullptr;
//            }
//            // alloc only
//            if (!p) return Alloc(newSize);
//
//            // begin realloc
//
//            // free part1
//            auto idx = *((size_t*)p - 1);
//
//            // calc original cap
//            auto cap = (size_t) ((size_t(1) << idx) - sizeof(void *));
//            if (cap >= newSize)
//                return p;
//
//            // new & copy data
//            auto np = Alloc(newSize);
//            memcpy(np, p, std::min(cap, dataLen));
//
//            // free part2
//            blocks[idx].push_back(p);
//
//            return np;
//        }
//    };
//}
//
//int main() {
//
//
////    xx::MemPool2 lmp;
////    auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize) {
////        return ((xx::MemPool2 *) ud)->Realloc(ptr, nsize, osize);
////    }, &lmp);
////    luaL_openlibs(L);
//
//    xx::Lua::State L;
//
//    luaL_dostring(L, R"(
//local bt
//local t = {}
//
//for j=1,3 do
//    bt = os.clock()
//    for i=1,1000000 do
//        t[#t+1] = {
//            {
//                a=1,b=2,c=3
//            }
//        }
//    end
//    print("insert cost", os.clock() - bt)
//
//    bt = os.clock()
//    for i=1,1000000 do
//        t[#t+1] = nil
//    end
//    print("delete cost", os.clock() - bt)
//
//    bt = os.clock()
//    collectgarbage()
//    print("gc cost", os.clock() - bt)
//
//end
//
//)");
//
//    std::cout << "end." << std::endl;
//    return 0;
//}
//
