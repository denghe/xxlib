#pragma once

#include "xx_data.h"
#include "xx_lua.h"

/*
使用 lua userdata 作为 xx::Data 内存管理的封装

C++ 注册:
	xx::Lua::Data::Register( L )

LUA 函数:
	NewXxData()

成员函数见 funcs 数组
*/

namespace xx::Lua {
	// 先适配指针版. 值版适配在最下面
	template<typename T>
	struct ToFuncs<T, std::enable_if_t<std::is_same_v<xx::Data*, std::decay_t<T>> || std::is_same_v<xx::Data const*, std::decay_t<T>>>> {
		static void To(lua_State* const& L, int const& idx, T& out) {
			if (lua_type(L, idx) != LUA_TUSERDATA) Error(L, "error! args[", std::to_string(idx), "] is not xx::Data");
			// todo: 进一步核实是否真的是 xx::Data
			out = (T)lua_touserdata(L, idx);
		}
	};
}

namespace xx::Lua::Data {
	inline auto key = "xxData";
	using D = xx::Data;

	inline int __gc(lua_State* L) {
		auto d = To<D*>(L);
		d->~D();
		return 0;
	}

	inline int __tostring(lua_State* L) {
		auto d = To<D*>(L);
		std::string s;
		s.reserve(d->len * 4);
		for (size_t i = 0; i < d->len; ++i) {
			s.append(std::to_string((*d)[i]));
			s.push_back(',');
		}
		if (d->len) {
			s.resize(s.size() - 1);
		}
		return Push(L, s);
	}

	inline int Ensure(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto siz = To<size_t>(L, 2);
		d->Reserve(d->len + siz);
		return 0;
	}

	inline int Reserve(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto cap = To<size_t>(L, 2);
		d->Reserve(cap);
		return 0;
	}

	inline int Resize(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto len = To<size_t>(L, 2);
		auto r = d->Resize(len);
		return Push(L, r);
	}

	inline int Clear(lua_State* L) {
		auto top = lua_gettop(L);
		assert(top >= 1);
		auto d = To<D*>(L);
		if (top == 1) {
			d->Clear();
		}
		else {
			bool b = To<bool>(L);
			d->Clear(b);
		}
		return 0;
	}

	inline int GetCap(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->cap);
	}

	inline int GetLen(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->len);
	}

	inline int SetLen(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto len = To<size_t>(L, 2);
		d->len = len;
		return 0;
	}

	inline int GetOffset(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->offset);
	}

	inline int SetOffset(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto offset = To<size_t>(L, 2);
		d->offset = offset;
		return 0;
	}

	inline int GetLeft(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->len - d->offset);
	}

	inline int At(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto idx = To<size_t>(L, 2);
		return Push(L, (*d)[idx]);
	}

	// 返回所有数据( buf, len, offset, cap )
	inline int GetAll(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->buf, d->len, d->offset, d->cap);
	}

	inline int Reset(lua_State* L) {
		auto top = lua_gettop(L);
		assert(top >= 1);
		auto d = To<D*>(L);
		switch (top) {
		case 1:
			d->Reset();
			break;
		case 2:
			d->Reset(To<uint8_t*>(L, 2));
			break;
		case 3:
			d->Reset(To<uint8_t*>(L, 2), To<size_t>(L, 3));
			break;
		case 4:
			d->Reset(To<uint8_t*>(L, 2), To<size_t>(L, 3), To<size_t>(L, 4));
			break;
		case 5:
			d->Reset(To<uint8_t*>(L, 2), To<size_t>(L, 3), To<size_t>(L, 4), To<size_t>(L, 5));
			break;
		default:
			throw std::exception("too many args");
		}
		return 0;
	}

	void AttachMT(lua_State* const& L);

	inline int Copy(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		lua_newuserdata(L, sizeof(D));							// ..., ud
		new(lua_touserdata(L, -1)) D(*d);
		AttachMT(L);											// ..., ud
		return 1;
	}

	inline int Equals(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto d2 = To<D*>(L, 2);
		auto r = ((*d) == (*d2));
		return Push(L, r);
	}

	inline int Fill(lua_State* L) {
		auto top = lua_gettop(L);
		assert(top >= 1);
		auto d = To<D*>(L);
		for (int idx = 2; idx <= top; ++idx) {
			auto v = To<uint8_t>(L, idx);
			d->WriteFixed(v);
		}
		return 0;
	}


	inline int Wj(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto siz = To<size_t>(L, 2);
		return Push(L, d->WriteJump(siz));
	}

	inline int Wbuf(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto s = To<std::string_view>(L, 2);
		d->WriteBuf(s.data(), s.size());
		return 0;
	}

	inline int Wbuf_at(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto idx = To<size_t>(L, 2);
		auto s = To<std::string_view>(L, 3);
		d->WriteBufAt(idx, s.data(), s.size());
		return 0;
	}

	// 写 lua string ( 变长长度 + 内容 )
	template<bool nullable = false>
	inline int Wstr(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		uint8_t f = 1;
		if constexpr (nullable) {
			if (lua_isnil(L, 2) || (lua_islightuserdata(L, 2) && !lua_touserdata(L, 2))) {
				f = 0;
			}
			d->WriteFixed(f);
		}
		if (f == 1) {
			auto s = To<std::string_view>(L, 2);
			d->WriteVarInteger(s.size());
			d->WriteBuf(s.data(), s.size());
		}
		return 0;
	}

	// 写 lua XxData ( 变长长度 + 内容 )
	template<bool nullable = false>
	inline int Wdata(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		D* d2 = nullptr;
		if constexpr (nullable) {
			if (lua_isnil(L, 2) || (lua_islightuserdata(L, 2) && !lua_touserdata(L, 2))) {
				d->WriteFixed((uint8_t)0);
				return 0;
			}
			else {
				d->WriteFixed((uint8_t)1);
			}
		}
		d2 = To<D*>(L, 2);
		if (d2) {
			d->WriteVarInteger(d2->len);
			d->WriteBuf(d2->buf, d2->len);
		}
		return 0;
	}

	// 从 Data 读 指定长度 buf 向 lua 压入 r, str( 如果 r 不为 0 则 str 为 nil )
	inline int Rbuf(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto len = To<size_t>(L, 2);
		if (auto ptr = (char const*)d->ReadBuf(len)) {
			return Push(L, 0, std::string_view(ptr, len));
		}
		return Push(L, __LINE__);
	}

	inline int Rbuf_at(lua_State* L) {
		assert(lua_gettop(L) == 3);
		auto d = To<D*>(L);
		auto idx = To<size_t>(L, 2);
		auto siz = To<size_t>(L, 3);
		if (auto ptr = (char const*)d->ReadBufAt(idx, siz)) {
			return Push(L, 0, std::string_view(ptr, siz));
		}
		return Push(L, __LINE__);
	}

	// 读 lua string ( 变长长度 + 内容 ). 返回值参考 Rbuf
	template<bool nullable = false>
	inline int Rstr(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		uint8_t f = 1;
		if constexpr (nullable) {
			if (int r = d->ReadFixed(f)) {
				return Push(L, r);
			}
		}
		if (f == 1) {
			size_t len;
			if (int r = d->ReadVarInteger(len)) {
				return Push(L, r);
			}
			if (auto ptr = (char const*)d->ReadBuf(len)) {
				return Push(L, 0, std::string_view(ptr, len));
			}
			return Push(L, __LINE__);
		}
		return Push(L, 0, nullptr);
	}

	int NewXxData(lua_State* L);

	// 读 lua XxData ( 变长长度 + 内容 ). 返回值参考 Rbuf
	template<bool nullable = false>
	inline int Rdata(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		uint8_t f = 1;
		if constexpr (nullable) {
			if (int r = d->ReadFixed(f)) {
				return Push(L, r);
			}
		}
		if (f == 1) {
			size_t len;
			if (int r = d->ReadVarInteger(len)) {
				return Push(L, r);
			}
			Push(L, 0);                                 // ..., 0
			NewXxData(L);                               // ..., 0, data
			auto d2 = (D*)lua_touserdata(L, -1);
			if (auto ptr = (char const*)d->ReadBuf(len)) {
				d2->WriteBuf(ptr, len);
				return 2;
			}
			lua_pop(L, 2);                              // ...
			return Push(L, __LINE__);
		}
		return Push(L, 0, nullptr);
	}

	// 读 lua XxData ( 变长长度 + 内容 ) 填充到 第二个参数的 XxData
	inline int RdataTo(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto d2 = To<D*>(L, 2);
		size_t len;
		if (int r = d->ReadVarInteger(len)) {
			return Push(L, r);
		}
		if (auto ptr = (char const*)d->ReadBuf(len)) {
			d2->Clear();
			d2->WriteBuf(ptr, len);
			return Push(L, 0);
		}
		return Push(L, __LINE__);
	}


	// 读参数并调用 d->WriteXxxx
	template<typename T, bool isFixed = true, bool isBE = false, bool isAt = false, bool nullable = false>
	inline int W(lua_State* L) {
		assert(lua_gettop(L) > 1);
		auto d = To<D*>(L);
		uint8_t f = 1;
		if constexpr (nullable) {
			if (lua_isnil(L, 2) || (lua_islightuserdata(L, 2) && !lua_touserdata(L, 2))) {
				f = 0;
			}
			d->WriteFixed(f);
		}
		if (f == 1) {
			if constexpr (isFixed) {
				if constexpr (isAt) {
					auto idx = To<size_t>(L, 2);
					auto v = To<T>(L, 3);
					if constexpr (isBE) {
						d->WriteFixedBEAt(idx, v);
					}
					else {
						d->WriteFixedAt(idx, v);
					}
				}
				else {
					auto v = To<T>(L, 2);
					if constexpr (isBE) {
						d->WriteFixedBE(v);
					}
					else {
						d->WriteFixed(v);
					}
				}
			}
			else {
				auto v = To<T>(L, 2);
				d->WriteVarInteger(v);
			}
		}
		return 0;
	}

	// 读参数并调用 d->ReadXxxx. 向 lua 压入 r, v( v 可能 nil, 可能不止一个值 )
	template<typename T, bool isFixed = true, bool isBE = false, bool isAt = false, bool nullable = false>
	inline int R(lua_State* L) {
		assert(lua_gettop(L) >= 1);
		auto d = To<D*>(L);
		uint8_t f = 1;
		if constexpr (nullable) {
			if (int r = d->ReadFixed(f)) {
				return Push(L, r);
			}
		}
		if (f == 1) {
			int r;
			T v;
			if constexpr (isFixed) {
				if constexpr (isAt) {
					auto idx = To<size_t>(L, 2);
					if constexpr (isBE) {
						r = d->ReadFixedBEAt(idx, v);
					}
					else {
						r = d->ReadFixedAt(idx, v);
					}
				}
				else {
					if constexpr (isBE) {
						r = d->ReadFixedBE(v);
					}
					else {
						r = d->ReadFixed(v);
					}
				}
			}
			else {
				r = d->ReadVarInteger(v);
			}
			if (r) return Push(L, r);
			return Push(L, r, v);
		}
		return Push(L, 0, nullptr);
	}

	inline luaL_Reg funcs[] = {
		{"__gc",       __gc},
		{"__tostring", __tostring},

		{"Fill",       Fill},
		{"Reset",      Reset},
		{"Copy",       Copy},
		{"Equals",     Equals},
		{"Ensure",     Ensure},
		{"Reserve",    Reserve},
		{"Resize",     Resize},
		{"Clear",      Clear},
		{"GetCap",     GetCap},
		{"GetLen",     GetLen},
		{"SetLen",     SetLen},
		{"GetOffset",  GetOffset},
		{"SetOffset",  SetOffset},
		{"GetLeft",    GetLeft},
		{"GetAll",     GetAll},
		{"At",         At},

		{"Wj",         Wj},
		{"Wbuf",       Wbuf},
		{"Wbuf_at",    Wbuf_at},
		{"Wstr",       Wstr},
		{"Wnstr",      Wstr<true>},
		{"Wdata",      Wdata},
		{"Wndata",     Wdata<true>},

		{"Wvi",        W<ptrdiff_t, false>},
		{"Wvu",        W<size_t, false>},

		{"Wb",         W<bool>},
		{"Wi8",        W<int8_t>},
		{"Wu8",        W<uint8_t>},
		{"Wi16",       W<int16_t>},
		{"Wu16",       W<uint16_t>},
		{"Wi32",       W<int32_t>},
		{"Wu32",       W<uint32_t>},
		{"Wi64",       W<int64_t>},
		{"Wu64",       W<uint64_t>},
		{"Wf",         W<float>},
		{"Wd",         W<double>},

		{"Wnvi",       W<ptrdiff_t, false, false, false, true>},
		{"Wnvu",       W<size_t, false, false, false, true>},

		{"Wnb",        W<bool, true, false, false, true>},
		{"Wni8",       W<int8_t, true, false, false, true>},
		{"Wnu8",       W<uint8_t, true, false, false, true>},
		{"Wni16",      W<int16_t, true, false, false, true>},
		{"Wnu16",      W<uint16_t, true, false, false, true>},
		{"Wni32",      W<int32_t, true, false, false, true>},
		{"Wnu32",      W<uint32_t, true, false, false, true>},
		{"Wni64",      W<int64_t, true, false, false, true>},
		{"Wnu64",      W<uint64_t, true, false, false, true>},
		{"Wnf",        W<float, true, false, false, true>},
		{"Wnd",        W<double, true, false, false, true>},

		{"Wi16_be",    W<int16_t, true, true>},
		{"Wu16_be",    W<uint16_t, true, true>},
		{"Wi32_be",    W<int32_t, true, true>},
		{"Wu32_be",    W<uint32_t, true, true>},
		{"Wi64_be",    W<int64_t, true, true>},
		{"Wu64_be",    W<uint64_t, true, true>},
		{"Wf_be",      W<float, true, true>},
		{"Wd_be",      W<double, true, true>},

		{"Wb_at",      W<bool, true, false, true>},
		{"Wi8_at",     W<int8_t, true, false, true>},
		{"Wu8_at",     W<uint8_t, true, false, true>},
		{"Wi16_at",    W<int16_t, true, false, true>},
		{"Wu16_at",    W<uint16_t, true, false, true>},
		{"Wi32_at",    W<int32_t, true, false, true>},
		{"Wu32_at",    W<uint32_t, true, false, true>},
		{"Wi64_at",    W<int64_t, true, false, true>},
		{"Wu64_at",    W<uint64_t, true, false, true>},
		{"Wf_at",      W<float, true, false, true>},
		{"Wd_at",      W<double, true, false, true>},

		{"Wi16_be_at", W<int16_t, true, true, true>},
		{"Wu16_be_at", W<uint16_t, true, true, true>},
		{"Wi32_be_at", W<int32_t, true, true, true>},
		{"Wu32_be_at", W<uint32_t, true, true, true>},
		{"Wi64_be_at", W<int64_t, true, true, true>},
		{"Wu64_be_at", W<uint64_t, true, true, true>},
		{"Wf_be_at",   W<float, true, true, true>},
		{"Wd_be_at",   W<double, true, true, true>},



		{"Rbuf",       Rbuf},
		{"Rbuf_at",    Rbuf_at},
		{"Rstr",       Rstr},
		{"Rnstr",      Rstr<true>},
		{"Rdata",      Rdata},
		{"RdataTo",    RdataTo},
		{"Rndata",     Rdata<true>},

		{"Rvi",        R<ptrdiff_t, false>},
		{"Rvu",        R<size_t, false>},

		{"Rb",         R<bool>},
		{"Ri8",        R<int8_t>},
		{"Ru8",        R<uint8_t>},
		{"Ri16",       R<int16_t>},
		{"Ru16",       R<uint16_t>},
		{"Ri32",       R<int32_t>},
		{"Ru32",       R<uint32_t>},
		{"Ri64",       R<int64_t>},
		{"Ru64",       R<uint64_t>},
		{"Rf",         R<float>},
		{"Rd",         R<double>},


		{"Rnvi",       R<ptrdiff_t, false, false, false, true>},
		{"Rnvu",       R<size_t, false, false, false, true>},

		{"Rnb",        R<bool, true, false, false, true>},
		{"Rni8",       R<int8_t, true, false, false, true>},
		{"Rnu8",       R<uint8_t, true, false, false, true>},
		{"Rni16",      R<int16_t, true, false, false, true>},
		{"Rnu16",      R<uint16_t, true, false, false, true>},
		{"Rni32",      R<int32_t, true, false, false, true>},
		{"Rnu32",      R<uint32_t, true, false, false, true>},
		{"Rni64",      R<int64_t, true, false, false, true>},
		{"Rnu64",      R<uint64_t, true, false, false, true>},
		{"Rnf",        R<float, true, false, false, true>},
		{"Rnd",        R<double, true, false, false, true>},


		{"Ri16_be",    R<int16_t, true, true>},
		{"Ru16_be",    R<uint16_t, true, true>},
		{"Ri32_be",    R<int32_t, true, true>},
		{"Ru32_be",    R<uint32_t, true, true>},
		{"Ri64_be",    R<int64_t, true, true>},
		{"Ru64_be",    R<uint64_t, true, true>},
		{"Rf_be",      R<float, true, true>},
		{"Rd_be",      R<double, true, true>},

		{"Rb_at",      R<bool, true, false, true>},
		{"Ri8_at",     R<int8_t, true, false, true>},
		{"Ru8_at",     R<uint8_t, true, false, true>},
		{"Ri16_at",    R<int16_t, true, false, true>},
		{"Ru16_at",    R<uint16_t, true, false, true>},
		{"Ri32_at",    R<int32_t, true, false, true>},
		{"Ru32_at",    R<uint32_t, true, false, true>},
		{"Ri64_at",    R<int64_t, true, false, true>},
		{"Ru64_at",    R<uint64_t, true, false, true>},
		{"Rf_at",      R<float, true, false, true>},
		{"Rd_at",      R<double, true, false, true>},

		{"Ri16_be_at", R<int16_t, true, true, true>},
		{"Ru16_be_at", R<uint16_t, true, true, true>},
		{"Ri32_be_at", R<int32_t, true, true, true>},
		{"Ru32_be_at", R<uint32_t, true, true, true>},
		{"Ri64_be_at", R<int64_t, true, true, true>},
		{"Ru64_be_at", R<uint64_t, true, true, true>},
		{"Rf_be_at",   R<float, true, true, true>},
		{"Rd_be_at",   R<double, true, true, true>},

		{nullptr,      nullptr}
	};

	// 创建 mt 到栈顶( 顺便存到注册表 )
	inline void CreateMT(lua_State* const& L) {
		CheckStack(L, 7);
		lua_createtable(L, 0, _countof(funcs));					// ..., mt
		lua_pushlightuserdata(L, (void*)key);					// ..., mt, key
		lua_pushvalue(L, -2);									// ..., mt, key, mt
		luaL_setfuncs(L, funcs, 0);								// ..., mt, key, mt
		lua_pushvalue(L, -1);									// ..., mt, key, mt, mt
		lua_setfield(L, -2, "__index");							// ..., mt, key, mt
		lua_pushvalue(L, -1);									// ..., mt, key, mt, mt
		lua_setfield(L, -2, "__metatable");						// ..., mt, key, mt
		lua_rawset(L, LUA_REGISTRYINDEX);						// ..., mt
	}

	// 从注册表拿 mt 附加给 ud
	inline void AttachMT(lua_State* const& L) {					// ..., ud
		CheckStack(L, 10);
		auto top = lua_gettop(L);
		lua_pushlightuserdata(L, (void*)key);					// ..., ud, key
		lua_rawget(L, LUA_REGISTRYINDEX);						// ..., ud, mt?
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);										// ..., ud
			CreateMT(L);										// ..., ud, mt
		}
		lua_setmetatable(L, -2);								// ..., ud
		assert(top == lua_gettop(L));
	}

	// 注册到 lua 全局, 创建 xx::Data userdata
	inline int NewXxData(lua_State* L) {
		lua_newuserdata(L, sizeof(D));							// ..., ud
		new(lua_touserdata(L, -1)) D();
		AttachMT(L);											// ..., ud
		return 1;
	}

	// 在 lua 中注册 全局的 Data 创建函数 和 注册表中的 metatable
	inline void Register(lua_State* const& L) {
		lua_pushcclosure(L, NewXxData, 0);						// ..., v
		lua_setglobal(L, "NewXxData");							// ...

		lua_pushlightuserdata(L, (void*)key);					// ..., key
		CreateMT(L);											// ..., key, mt
		lua_rawset(L, LUA_REGISTRYINDEX);						// ...
	}
}

namespace xx::Lua {
	// 复制或移动
	template<typename T>
	struct PushFuncs<T, std::enable_if_t<std::is_same_v<xx::Data, std::decay_t<T>>>> {
		static int Push(lua_State* const& L, T&& in) {
			auto rtv = ::xx::Lua::Data::NewXxData(L);
			auto d = To<xx::Data*>(L, -1);
			*d = std::forward<T>(in);
			return rtv;
		}
	};
}
