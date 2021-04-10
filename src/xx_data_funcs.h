#pragma once
#include "xx_data.h"
#include "xx_helpers.h"

namespace xx {
	/**********************************************************************************************************************/
	// 为 Data.Read / Write 提供简单序列化功能( 1字节整数, float/double  memcpy,   别的整数 包括长度 变长. std::string/xx::Data 写 长度 + 内容
	// 这里只针对 “基础类型”

	// 适配 Data
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_base_of_v<Data, T>>> {
		static inline void Write(Data& dw, T const& in) {
			dw.WriteVarInteger(in.len);
			dw.WriteBuf(in.buf, in.len);
		}
		static inline int Read(Data_r& dr, T& out) {
			size_t siz = 0;
			if (int r = dr.ReadVarInteger(siz)) return r;
			if (dr.offset + siz > dr.len) return __LINE__;
			out.Clear();
			out.WriteBuf(dr.buf + dr.offset, siz);
			dr.offset += siz;
			return 0;
		}
	};

	// 适配 1 字节长度的 数值 或 float/double( 这些类型直接 memcpy )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< (std::is_arithmetic_v<T> && sizeof(T) == 1) || std::is_floating_point_v<T> >> {
		static inline void Write(Data& dw, T const& in) {
			dw.WriteFixed(in);
		}
		static inline int Read(Data_r& dr, T& out) {
			return dr.ReadFixed(out);
		}
	};

	// 适配 2+ 字节整数( 变长读写 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) >= 2>> {
		static inline void Write(Data& dw, T const& in) {
			dw.WriteVarInteger(in);
		}
		static inline int Read(Data_r& dr, T& out) {
			return dr.ReadVarInteger(out);
		}
	};

	// 适配 enum( 根据原始数据类型调上面的适配 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
		typedef std::underlying_type_t<T> UT;
		static inline void Write(Data& dw, T const& in) {
			dw.Write((UT const&)in);
		}
		static inline int Read(Data_r& dr, T& out) {
			return dr.Read((UT&)out);
		}
	};

	// 适配 std::string_view ( 写入 变长长度 + 内容 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
		static inline void Write(Data& dw, T const& in) {
			dw.WriteVarInteger(in.size());
			dw.WriteBuf((char*)in.data(), in.size());
		}
		static inline int Read(Data_r& dr, T& out) {
			size_t siz = 0;
			if (auto r = dr.ReadVarInteger(siz)) return r;
			if (auto buf = dr.ReadBuf(siz)) {
				out = std::string_view((char*)buf, siz);
				return 0;
			}
			return __LINE__;
		}
	};

	// 适配 literal char[len] string  ( 写入 变长长度-1 + 内容. 不写入末尾 0 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
		static inline void Write(Data& dw, T const& v) {
			DataFuncs<std::string_view, void>::Write(dw, std::string_view(v));
		}
	};

	// 适配 std::string ( 写入 变长长度 + 内容 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
		static inline void Write(Data& dw, T const& in) {
			DataFuncs<std::string_view, void>::Write(dw, std::string_view(in));
		}
		static inline int Read(Data_r& dr, T& out) {
			size_t siz = 0;
			if (auto r = dr.ReadVarInteger(siz)) return r;
			if (dr.offset + siz > dr.len) return __LINE__;
			out.assign((char*)dr.buf + dr.offset, siz);
			dr.offset += siz;
			return 0;
		}
	};


	// 上面就是所谓 基础类型 的所有适配. 下面针对常用容器，做 只含有 简单类型 的容器 的适配. 这样一来, xx_obj 那边 就可以用排除法 写扩展，这边的适配模板，可继续沿用

	template<typename T, typename ENABLED = void> struct IsBaseDataType : std::false_type {};
	template<typename T> constexpr bool IsBaseDataType_v = IsBaseDataType<T>::value;

	template<typename T> struct IsBaseDataType<T, std::enable_if_t< std::is_base_of_v<Data, T> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< (std::is_arithmetic_v<T> && sizeof(T) == 1) || std::is_floating_point_v<T> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< std::is_integral_v<T> && sizeof(T) >= 2 >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< std::is_enum_v<T> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< std::is_same_v<std::string_view, std::decay_t<T>> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsLiteral_v<T> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< std::is_same_v<std::string, std::decay_t<T>> >> : std::true_type {};

	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsOptional_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsVector_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsSetSeries_v<T>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsPair_v<T>&& IsBaseDataType_v<typename T::first_type>&& IsBaseDataType_v<typename T::second_type> >> : std::true_type {};
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsMapSeries_v<T>&& IsBaseDataType_v<typename T::key_type>&& IsBaseDataType_v<typename T::value_type> >> : std::true_type {};
	
	// tuple 得遍历每个类型来判断
	template<std::size_t index, typename Tuple>
	constexpr bool TupleIsBaseDataType__() {
		if constexpr (index == std::tuple_size_v<Tuple>) {
			return true;
		}
		else {
			return IsBaseDataType_v<std::tuple_element_t<index, Tuple>> ? TupleIsBaseDataType__<index + 1, Tuple>() : false;
		}
	}
	template<typename Tuple>
	constexpr bool TupleIsBaseDataType() {
		return TupleIsBaseDataType__<0, Tuple>();
	}
	template<typename T> struct IsBaseDataType<T, std::enable_if_t< IsTuple_v<T>&& TupleIsBaseDataType<T>() >> : std::true_type {};


	// 适配 std::optional<T>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsBaseDataType_v<T>&& IsOptional_v<T>>> {
		static inline void Write(Data& dw, T const& in) {
			if (in.has_value()) {
				dw.Write((char)1, in.value());
			}
			else {
				dw.Write((char)0);
			}
		}
		static inline int Read(Data_r& dr, T& out) {
			char hasValue = 0;
			if (int r = dr.Read(hasValue)) return r;
			if (!hasValue) return 0;
			if (!out.has_value()) {
				out.emplace();
			}
			return dr.Read(out.value());
		}
	};

	// 适配 std::pair<K, V>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsBaseDataType_v<T>&& IsPair_v<T>>> {
		static inline void Write(Data& dw, T const& in) {
			dw.Write(in.first, in.second);
		}
		static inline int Read(Data_r& dr, T& out) {
			return dr.Read(out.first, out.second);
		}
	};
}
