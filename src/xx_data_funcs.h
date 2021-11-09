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
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteVarInteger<needReserve>(in.len);
			d.WriteBuf<needReserve>(in.buf, in.len);
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (int r = d.ReadVarInteger(siz)) return r;
			if (d.offset + siz > d.len) return __LINE__;
			out.Clear();
			out.WriteBuf(d.buf + d.offset, siz);
			d.offset += siz;
			return 0;
		}
	};
	
	// 适配 Span ( do not write len )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_same_v<Span, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteBuf<needReserve>(in.buf, in.len);
		}
	};

	// 适配 1 字节长度的 数值( 含 double ) 或 float/double( 这些类型直接 memcpy )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< (std::is_arithmetic_v<T> && sizeof(T) == 1) || std::is_floating_point_v<T> >> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteFixed<needReserve>(in);
		}
		static inline int Read(Data_r& d, T& out) {
			return d.ReadFixed(out);
		}
	};

	// 适配 2+ 字节整数( 变长读写 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) >= 2>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteVarInteger<needReserve>(in);
		}
		static inline int Read(Data_r& d, T& out) {
			return d.ReadVarInteger(out);
		}
	};

	// 适配 enum( 根据原始数据类型调上面的适配 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
		typedef std::underlying_type_t<T> UT;
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.Write<needReserve>((UT const&)in);
		}
		static inline int Read(Data_r& d, T& out) {
			return d.Read((UT&)out);
		}
	};

	// 适配 std::string_view ( 写入 变长长度 + 内容 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteVarInteger<needReserve>(in.size());
			d.WriteBuf<needReserve>((char*)in.data(), in.size());
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (auto r = d.ReadVarInteger(siz)) return r;
			if (auto buf = d.ReadBuf(siz)) {
				out = std::string_view((char*)buf, siz);
				return 0;
			}
			return __LINE__;
		}
	};

	// 适配 literal char[len] string  ( 写入 变长长度-1 + 内容. 不写入末尾 0 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			DataFuncs<std::string_view, void>::Write<needReserve>(d, std::string_view(in));
		}
	};

	// 适配 std::string ( 写入 变长长度 + 内容 )
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			DataFuncs<std::string_view, void>::Write<needReserve>(d, std::string_view(in));
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (auto r = d.ReadVarInteger(siz)) return r;
			if (d.offset + siz > d.len) return __LINE__;
			out.assign((char*)d.buf + d.offset, siz);
			d.offset += siz;
			return 0;
		}
	};


	// 适配 std::optional<T>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsOptional_v<T>/* && IsBaseDataType_v<T>*/>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			if (in.has_value()) {
				d.Write<needReserve>((uint8_t)1, in.value());
			}
			else {
				d.Write<needReserve>((uint8_t)0);
			}
		}
		static inline int Read(Data_r& d, T& out) {
			char hasValue = 0;
			if (int r = d.Read(hasValue)) return r;
			if (!hasValue) return 0;
			if (!out.has_value()) {
				out.emplace();
			}
			return d.Read(out.value());
		}
	};

	// 适配 std::pair<K, V>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsPair_v<T>/* && IsBaseDataType_v<T>*/>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.Write<needReserve>(in.first, in.second);
		}
		static inline int Read(Data_r& d, T& out) {
			return d.Read(out.first, out.second);
		}
	};

	// 适配 std::tuple<......>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsTuple_v<T>/* && IsBaseDataType_v<T>*/>> {
	    template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            std::apply([&](auto const &... args) {
                d.Write<needReserve>(args...);
            }, in);
        }

        template<std::size_t I = 0, typename... Tp>
        static std::enable_if_t<I == sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
            return d.Read(std::get<I>(t));
        }

        template<std::size_t I = 0, typename... Tp>
        static std::enable_if_t < I < sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
            if (int r = d.Read(std::get<I>(t))) return r;
            return ReadTuple<I + 1, Tp...>(d, t);
        }

        static inline int Read(Data_r& d, T& out) {
            return ReadTuple(d, out);
        }
	};


	// 适配 std::variant<......>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsVariant_v<T>>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			std::visit([&](auto&& v) {
				d.Write<needReserve>((size_t)in.index());
				d.Write<needReserve>(std::forward<decltype(v)>(v));
			}, in);
		}

		template<std::size_t I = 0, typename... Ts>
		static std::enable_if_t<I == sizeof...(Ts) - 1, int> ReadVariant(Data_r& d, std::variant<Ts...>& t, size_t const& index) {
			if (I == index) {
				t = std::tuple_element_t<I, std::tuple<Ts...>>();
				int r;
				std::visit([&](auto& v) {
					r = d.Read(v);
				}, t);
				return r;
			}
			else return -1;
		}
		template<std::size_t I = 0, typename... Ts>
		static std::enable_if_t < I < sizeof...(Ts) - 1, int> ReadVariant(Data_r& d, std::variant<Ts...>& t, size_t const& index) {
			if (I == index) {
				t = std::tuple_element_t<I, std::tuple<Ts...>>();
				int r;
				std::visit([&](auto& v) {
					r = d.Read(v);
				}, t);
				return r;
			}
			else return ReadVariant<I + 1, Ts...>(d, t, index);
		}

		static inline int Read(Data_r& d, T& out) {
			size_t index;
			if (int r = d.Read(index)) return r;
			return ReadVariant(d, out, index);
		}
	};


	// 适配 std::vector, std::array   // todo: queue / deque
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< (IsVector_v<T> || IsArray_v<T>)/* && IsBaseDataType_v<T>*/>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
		    if constexpr(IsVector_v<T>) {
                d.WriteVarInteger<needReserve>(in.size());
                if (in.empty()) return;
            }
			if constexpr (sizeof(T) == 1 || std::is_floating_point_v<T>) {
				d.WriteFixedArray<needReserve>(in.data(), in.size());
			}
			else if constexpr (std::is_integral_v<typename T::value_type>) {
				if constexpr (needReserve) {
					auto cap = in.size() * (sizeof(T) + 1);
					if (d.cap < cap) {
						d.Reserve<false>(cap);
					}
				}
				for (auto&& o : in) {
					d.WriteVarInteger<false>(o);
				}
			}
			else {
				for (auto&& o : in) {
					d.Write<needReserve>(o);
				}
			}
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
            if constexpr(IsVector_v<T>) {
                if (int r = d.ReadVarInteger(siz)) return r;
                if (d.offset + siz > d.len) return __LINE__;
                out.resize(siz);
                if (siz == 0) return 0;
            }
            else {
                siz = out.size();
            }
			auto buf = out.data();
			if constexpr (sizeof(T) == 1 || std::is_floating_point_v<T>) {
				if (int r = d.ReadFixedArray(buf, siz)) return r;
			}
			else {
				for (size_t i = 0; i < siz; ++i) {
					if (int r = d.Read(buf[i])) return r;
				}
			}
			return 0;
		}
	};

	// 适配 std::set, unordered_set
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< IsSetSeries_v<T>/* && IsBaseDataType_v<T>*/>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteVarInteger<needReserve>(in.size());
			if (in.empty()) return;
			if constexpr (std::is_integral_v<typename T::value_type>) {
				if constexpr (needReserve) {
					auto cap = in.size() * (sizeof(T) + 1);
					if (d.cap < cap) {
						d.Reserve<false>(cap);
					}
				}
				for (auto&& o : in) {
					d.WriteVarInteger<false>(o);
				}
			}
			else {
				for (auto&& o : in) {
					d.Write<needReserve>(o);
				}
			}
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz = 0;
			if (int r = d.Read(siz)) return r;
			if (d.offset + siz > d.len) return __LINE__;
			out.clear();
			if (siz == 0) return 0;
			for (size_t i = 0; i < siz; ++i) {
				if (int r = Read_(d, out.emplace())) return r;
			}
			return 0;
		}
	};

	// 适配 std::map unordered_map
	template<typename T>
	struct DataFuncs<T, std::enable_if_t< IsMapSeries_v<T> /*&& IsBaseDataType_v<T>*/>> {
		template<bool needReserve = true>
		static inline void Write(Data& d, T const& in) {
			d.WriteVarInteger<needReserve>(in.size());
			for (auto&& kv : in) {
				d.Write<needReserve>(kv.first, kv.second);
			}
		}
		static inline int Read(Data_r& d, T& out) {
			size_t siz;
			if (int r = d.Read(siz)) return r;
			if (siz == 0) return 0;
			if (d.offset + siz * 2 > d.len) return __LINE__;
			for (size_t i = 0; i < siz; ++i) {
				MapSeries_Pair_t<T> kv;
				if (int r = d.Read(kv.first, kv.second)) return r;
				out.insert(std::move(kv));
			}
			return 0;
		}
	};

}
