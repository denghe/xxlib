#pragma once

#include "xx_lua_randoms.h"
#include "xx_data.h"

namespace xx::Lua {

    // 将栈顶的数据写入 Data (不支持 table 循环引用, 注意： lua5.1 不支持 int64, 这里并没有走 userdata 规则)
    template<typename T>
    static inline void WriteTo(lua_State* const& L, xx::Data& d, T const& in) {
        switch (auto t = lua_type(in, -1)) {
        case LUA_TNIL:
            d.WriteFixed(ValueTypes::NilType);
            return;
        case LUA_TBOOLEAN:
            d.WriteFixed(lua_toboolean(in, -1) ? ValueTypes::True : ValueTypes::False);
            return;
        case LUA_TNUMBER: {
#if LUA_VERSION_NUM == 501
            auto n = (double)lua_tonumber(in, -1);
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
            if (lua_isinteger(in, -1)) {
                d.WriteFixed(ValueTypes::Integer);
                d.WriteVarInteger((int64_t)lua_tointeger(in, -1));
            }
            else {
                d.WriteFixed(ValueTypes::Double);
                d.WriteFixed((double)lua_tonumber(in, -1));
            }
#endif
            return;
        }
        case LUA_TSTRING: {
            size_t len;
            auto ptr = lua_tolstring(in, -1, &len);
            d.WriteFixed(ValueTypes::String);
            d.WriteVarInteger(len);
            d.WriteBuf(ptr, len);
            return;
        }
        case LUA_TTABLE: {
            d.WriteFixed(ValueTypes::Table);
            int idx = lua_gettop(in);                                   // 存 idx 备用
            CheckStack(in, 1);
            lua_pushnil(in);                                            //                      ... t, nil
            while (lua_next(in, idx) != 0) {                            //                      ... t, k, v
                // 先检查下 k, v 是否为不可序列化类型. 如果是就 next
                t = lua_type(in, -1);
                if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TTHREAD) {
                    lua_pop(in, 1);                                     //                      ... t, k
                    continue;
                }
                t = lua_type(in, -2);
                if (t == LUA_TFUNCTION || t == LUA_TLIGHTUSERDATA || t == LUA_TTHREAD) {
                    lua_pop(in, 1);                                     //                      ... t, k
                    continue;
                }
                Write(d, in);											// 先写 v
                lua_pop(in, 1);                                         //                      ... t, k
                Write(d, in);											// 再写 k
            }
            d.WriteFixed(ValueTypes::TableEnd);
            return;
        }
        case LUA_TUSERDATA: {
            // todo: 支持内部结构体 例如 Random*
            // 拿 mt, 拿 typeId, 再 switch case 写死? 或者走 模板适配?

        }
        default:
            d.WriteFixed(ValueTypes::NilType);
            return;
        }
    }

    // 从 Data 读出一个 lua value 压入 L 栈顶
    // 如果读失败, 可能会有残留数据已经压入，外界需自己做 lua state 的 cleanup
    template<typename T>
    static inline int ReadFrom(lua_State* const& L, xx::Data_r& d, T& out) {
        ValueTypes lt;
        if (int r = d.ReadFixed(lt)) return r;
        switch (lt) {
        case ValueTypes::NilType:
            CheckStack(out, 1);
            lua_pushnil(out);
            return 0;
        case ValueTypes::True:
            CheckStack(out, 1);
            lua_pushboolean(out, 1);
            return 0;
        case ValueTypes::False:
            CheckStack(out, 1);
            lua_pushboolean(out, 0);
            return 0;
        case ValueTypes::Integer: {
            int64_t i;
            if (int r = d.ReadVarInteger(i)) return r;
            CheckStack(out, 1);
            lua_pushinteger(out, (lua_Integer)i);
            return 0;
        }
        case ValueTypes::Double: {
            lua_Number v;
            if (int r = d.ReadFixed(v)) return r;
            CheckStack(out, 1);
            lua_pushnumber(out, v);
            return 0;
        }
        case ValueTypes::String: {
            size_t len;
            if (int r = d.ReadVarInteger(len)) return r;
            if (d.offset + len >= d.len) return __LINE__;
            CheckStack(out, 1);
            lua_pushlstring(out, (char*)d.buf + d.offset, len);
            d.offset += len;
            return 0;
        }
        case ValueTypes::Userdata: {
            // todo: 支持内部结构体 例如 Random*
        }
        case ValueTypes::Table: {
            CheckStack(out, 4);
            lua_newtable(out);                                          // ... t
            while (d.offset < d.len) {
                if (d.buf[d.offset] == (char)ValueTypes::TableEnd) {
                    ++d.offset;
                    return 0;
                }
                if (int r = Read(d, out)) return r;						// ... t, v
                if (int r = Read(d, out)) return r;						// ... t, v, k
                lua_pushvalue(out, -2);                                 // ... t, v, k, v
                lua_rawset(out, -4);                                    // ... t, v
                lua_pop(out, 1);                                        // ... t
            }
        }
        default:
            return __LINE__;
        }
    }
}
