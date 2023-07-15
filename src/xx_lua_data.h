#pragma once

/*
xx::Data 主体映射到 lua

C++ 注册:
	xx::Lua::Data::Register( L )

LUA 全局函数( 可修改 ):
	NewXxData()

成员函数见 funcs 数组

Push, To 适配及其用法 & 注意事项，见本文件最下方代码

*/

#include "xx_data.h"
#include "xx_lua_bind.h"

namespace xx::Lua::Data {
	using D = xx::Data;
	using SIZ_t =
#if LUA_VERSION_NUM == 501
        uint32_t;
#else
	    size_t;
#endif

	// 在 lua 中注册 全局的 Data 创建函数
	inline void Register(lua_State* const& L, char const* const& keyName = "xxData") {
		SetGlobalCClosure(L, keyName, [](auto L)->int { return PushUserdata<D>(L); });
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
		auto siz = To<SIZ_t>(L, 2);
		d->Reserve(d->len + siz);
		return 0;
	}

	inline int Reserve(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto cap = To<SIZ_t>(L, 2);
		d->Reserve(cap);
		return 0;
	}

	inline int Resize(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto len = To<SIZ_t>(L, 2);
		auto r = d->Resize(len);
		return Push(L, (SIZ_t)r);
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
		return Push(L, (SIZ_t)d->cap);
	}

	inline int GetLen(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, (SIZ_t)d->len);
	}

	inline int SetLen(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto len = To<SIZ_t>(L, 2);
		d->len = len;
		return 0;
	}

	inline int GetOffset(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, (SIZ_t)d->offset);
	}

	inline int SetOffset(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto offset = To<SIZ_t>(L, 2);
		d->offset = offset;
		return 0;
	}

	inline int GetLeft(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, (SIZ_t)(d->len - d->offset));
	}

	inline int At(lua_State* L) {
		assert(lua_gettop(L) == 2);
		auto d = To<D*>(L);
		auto idx = To<SIZ_t>(L, 2);
		return Push(L, (*d)[idx]);
	}

	// 返回所有数据( buf, len, offset, cap )
	inline int GetAll(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		return Push(L, d->buf, (SIZ_t)d->len, (SIZ_t)d->offset, (SIZ_t)d->cap);
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
			d->Reset(To<uint8_t*>(L, 2), To<SIZ_t>(L, 3));
			break;
		case 4:
			d->Reset(To<uint8_t*>(L, 2), To<SIZ_t>(L, 3), To<SIZ_t>(L, 4));
			break;
		case 5:
			d->Reset(To<uint8_t*>(L, 2), To<SIZ_t>(L, 3), To<SIZ_t>(L, 4), To<SIZ_t>(L, 5));
			break;
		default:
			throw std::runtime_error("too many args");
		}
		return 0;
	}


	inline int Copy(lua_State* L) {
		assert(lua_gettop(L) == 1);
		auto d = To<D*>(L);
		PushUserdata<D>(L, *d);
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
		auto siz = To<SIZ_t>(L, 2);
		return Push(L, (SIZ_t)d->WriteJump(siz));
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
		auto idx = To<SIZ_t>(L, 2);
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
		auto len = To<SIZ_t>(L, 2);
		if (auto ptr = (char const*)d->ReadBuf(len)) {
			return Push(L, 0, std::string_view(ptr, len));
		}
		return Push(L, __LINE__);
	}

	inline int Rbuf_at(lua_State* L) {
		assert(lua_gettop(L) == 3);
		auto d = To<D*>(L);
		auto idx = To<SIZ_t>(L, 2);
		auto siz = To<SIZ_t>(L, 3);
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
			PushUserdata<D>(L);							// ..., 0, data
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
					auto idx = To<SIZ_t>(L, 2);
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
					auto idx = To<SIZ_t>(L, 2);
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

		{"Wvi",        W<int64_t, false>},
		{"Wvu",        W<uint64_t, false>},
		{"Wvi64",      W<int64_t, false>},
		{"Wvu64",      W<uint64_t, false>},
		{"Wvi32",      W<int32_t, false>},
		{"Wvu32",      W<uint32_t, false>},
		{"Wvi16",      W<int16_t, false>},
		{"Wvu16",      W<uint16_t, false>},

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

		{"Wnvi",       W<int64_t, false, false, false, true>},
		{"Wnvu",       W<uint64_t, false, false, false, true>},
		{"Wnvi64",     W<int64_t, false, false, false, true>},
		{"Wnvu64",     W<uint64_t, false, false, false, true>},
		{"Wnvi32",     W<int32_t, false, false, false, true>},
		{"Wnvu32",     W<uint32_t, false, false, false, true>},
		{"Wnvi16",     W<int16_t, false, false, false, true>},
		{"Wnvu16",     W<uint16_t, false, false, false, true>},

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

		{"Rvi",        R<int64_t, false>},
		{"Rvu",        R<uint64_t, false>},
		{"Rvi64",      R<int64_t, false>},
		{"Rvu64",      R<uint64_t, false>},
		{"Rvi32",      R<int32_t, false>},
		{"Rvu32",      R<uint32_t, false>},
		{"Rvi16",      R<int16_t, false>},
		{"Rvu16",      R<uint16_t, false>},

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


		{"Rnvi",       R<int64_t, false, false, false, true>},
		{"Rnvu",       R<uint64_t, false, false, false, true>},
		{"Rnvi64",     R<int64_t, false, false, false, true>},
		{"Rnvu64",     R<uint64_t, false, false, false, true>},
		{"Rnvi32",     R<int32_t, false, false, false, true>},
		{"Rnvu32",     R<uint32_t, false, false, false, true>},
		{"Rnvi16",     R<int16_t, false, false, false, true>},
		{"Rnvu16",     R<uint16_t, false, false, false, true>},

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

        // for luajit force u/int64 <-> double
        {"Rvi64d",     R<di64_t, false>},
        {"Rvu64d",     R<du64_t, false>},
        {"Wvi64d",     W<di64_t, false>},
        {"Wvu64d",     W<du64_t, false>},

        {nullptr,      nullptr}
	};
}

namespace xx::Lua {
	// meta
	template<typename T>
	struct MetaFuncs<T, std::enable_if_t<std::is_same_v<xx::Data, std::decay_t<T>>>> {
		using U = xx::Data;
		inline static std::string name = TypeName<U>();
		static void Fill(lua_State* const& L) {
			SetType<U>(L);
			luaL_setfuncs(L, ::xx::Lua::Data::funcs, 0);
		}
	};
	// push ( 移动或拷贝 ), to ( 数据会挪走 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_same_v<xx::Data, std::decay_t<T>>>> {
		static constexpr int checkStackSize = 1;
		static int Push_(lua_State* const& L, T&& in) {
			return PushUserdata<xx::Data>(L, std::forward<T>(in));
		}
		static void To_(lua_State* const& L, int const& idx, T& out) {
			AssertType<xx::Data>(L, idx);
			out = std::move(*(T*)lua_touserdata(L, idx));
		}
	};
	// 指针方式 to ( 不希望挪走数据 就用这个 )
	template<typename T>
	struct PushToFuncs<T, std::enable_if_t<std::is_pointer_v<std::decay_t<T>> && std::is_same_v<xx::Data, std::decay_t<std::remove_pointer_t<std::decay_t<T>>>>>> {
		static void To_(lua_State* const& L, int const& idx, T& out) {
			AssertType<xx::Data>(L, idx);
			out = (T)lua_touserdata(L, idx);
		}
	};
}
