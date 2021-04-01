#pragma once
#include "xx_data.h"
#include "xx_helpers.h"

namespace xx {
	/**********************************************************************************************************************/
	// 为 Data.Read / Write 提供简单序列化功能( 1字节整数, float/double  memcpy,   别的整数 包括长度 变长. std::string/xx::Data 写 长度 + 内容

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

	// 适配 std::optional<T>
	template<typename T>
	struct DataFuncs<T, std::enable_if_t<IsOptional_v<T>>> {
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
    struct DataFuncs<T, std::enable_if_t<IsPair_v<T>>> {
        static inline void Write(Data& dw, T const& in) {
            dw.Write(in.first, in.second);
        }
        static inline int Read(Data_r& dr, T& out) {
            return dr.Read(out.first, out.second);
        }
    };
}
