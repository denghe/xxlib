#pragma once
#include "xx_includes.h"
#include "xx_typetraits.h"

namespace xx {
    // 各种 string 辅助( 主要针对基础数据类型或简单自定义结构 )

    /************************************************************************************/
    // StringFuncs 适配模板 for easy to append value to std::string
    /************************************************************************************/

    template<typename T, typename ENABLED = void>
    struct StringFuncs {
        static inline void Append(std::string& s, T const& in) {
            assert(false);
        }
        static inline void AppendCore(std::string& s, T const& in) {
            assert(false);
        }
    };

    /************************************************************************************/
    // Append / ToString
    /************************************************************************************/

    namespace Core {
        template<typename T>
        void Append(std::string &s, T const &v) {
            ::xx::StringFuncs<T>::Append(s, v);
        }
    }

    template<typename ...Args>
    void Append(std::string& s, Args const& ... args) {
        (::xx::Core::Append(s, args), ...);
    }

    template<std::size_t...Idxs, typename...TS>
    void AppendFormatCore(std::index_sequence<Idxs...>, std::string& s, size_t const& i, TS const&...vs) {
        (((i == Idxs) ? (Append(s, vs), 0) : 0), ...);
    }

    // 格式化追加, {0} {1}... 这种. 针对重复出现的参数, 是从已经追加出来的字串区域复制, 故追加自己并不会导致内容翻倍
    template<typename...TS>
    size_t AppendFormat(std::string& s, char const* const& format, TS const&...vs) {
        std::array<std::pair<size_t, size_t>, sizeof...(vs)> cache{};
        size_t offset = 0;
        while (auto c = format[offset]) {
            if (c == '{') {
                c = format[++offset];
                if (c == '{') {
                    Append(s, '{');
                }
                else {
                    size_t i = 0;
                    while ((c = format[offset])) {
                        if (c == '}') {
                            if (i >= sizeof...(vs)) return i;   // error
                            if (cache[i].second) {
                                s.append(s.data() + cache[i].first, cache[i].second);
                            }
                            else {
                                cache[i].first = s.size();
                                AppendFormatCore(std::make_index_sequence<sizeof...(TS)>(), s, i, vs...);
                                cache[i].second = s.size() - cache[i].first;
                            }
                            break;
                        }
                        else {
                            i = i * 10 + (c - '0');
                        }
                        ++offset;
                    }
                }
            }
            else {
                Append(s, c);
            }
            ++offset;
        }
        return 0;
    }

    template<typename ...Args>
    std::string ToString(Args const& ... args) {
        std::string s;
        Append(s, args...);
        return s;
    }

    template<typename ...Args>
    std::string ToStringFormat(char const* const& format, Args const& ... args) {
        std::string s;
        AppendFormat(s, format, args...);
        return s;
    }



    // ucs4 to utf8. write to out. return len
    inline size_t Char32ToUtf8(char32_t const& c32, char* out) {
        auto& c = (uint32_t&)c32;
        auto& o = (uint8_t*&)out;
        if (c < 0x7F) {
            o[0] = c;
            return 1;
        }
        else if (c < 0x7FF) {
            o[0] = 0b1100'0000 | (c >> 6);
            o[1] = 0b1000'0000 | (c & 0b0011'1111);
            return 2;
        }
        else if (c < 0x10000) {
            o[0] = 0b1110'0000 | (c >> 12);
            o[1] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[2] = 0b1000'0000 | (c & 0b0011'1111);
            return 3;
        }
        else if (c < 0x110000) {
            o[0] = 0b1111'0000 | (c >> 18);
            o[1] = 0b1000'0000 | ((c >> 12) & 0b0011'1111);
            o[2] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[3] = 0b1000'0000 | (c & 0b0011'1111);
            return 4;
        }
        else if (c < 0x1110000) {
            o[0] = 0b1111'1000 | (c >> 24);
            o[1] = 0b1000'0000 | ((c >> 18) & 0b0011'1111);
            o[2] = 0b1000'0000 | ((c >> 12) & 0b0011'1111);
            o[3] = 0b1000'0000 | ((c >> 6) & 0b0011'1111);
            o[4] = 0b1000'0000 | (c & 0b0011'1111);
            return 4;
        }
        throw std::logic_error("out of char32_t handled range");
    }



    inline std::u32string StringU8ToU32(std::string_view const& s) {
        std::u32string ws;
        ws.reserve(s.size());
        char32_t wc{};
        for (int i = 0; i < s.size(); ) {
            char c = s[i];
            if ((c & 0x80) == 0) {
                wc = c;
                ++i;
            } else if ((c & 0xE0) == 0xC0) {
                wc = (s[i] & 0x1F) << 6;
                wc |= (s[i + 1] & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                wc = (s[i] & 0xF) << 12;
                wc |= (s[i + 1] & 0x3F) << 6;
                wc |= (s[i + 2] & 0x3F);
                i += 3;
            } else if ((c & 0xF8) == 0xF0) {
                wc = (s[i] & 0x7) << 18;
                wc |= (s[i + 1] & 0x3F) << 12;
                wc |= (s[i + 2] & 0x3F) << 6;
                wc |= (s[i + 3] & 0x3F);
                i += 4;
            } else if ((c & 0xFC) == 0xF8) {
                wc = (s[i] & 0x3) << 24;
                wc |= (s[i] & 0x3F) << 18;
                wc |= (s[i] & 0x3F) << 12;
                wc |= (s[i] & 0x3F) << 6;
                wc |= (s[i] & 0x3F);
                i += 5;
            } else if ((c & 0xFE) == 0xFC) {
                wc = (s[i] & 0x1) << 30;
                wc |= (s[i] & 0x3F) << 24;
                wc |= (s[i] & 0x3F) << 18;
                wc |= (s[i] & 0x3F) << 12;
                wc |= (s[i] & 0x3F) << 6;
                wc |= (s[i] & 0x3F);
                i += 6;
            }
            ws += wc;
        }
        return ws;
    }

    inline std::string StringU32ToU8(std::u32string_view const& ws) {
        std::string s;
        for (int i = 0; i < ws.size(); ++i) {
            char32_t wc = ws[i];
            if (0 <= wc && wc <= 0x7f) {
                s += (char)wc;
            } else if (0x80 <= wc && wc <= 0x7ff) {
                s += (0xc0 | (wc >> 6));
                s += (0x80 | (wc & 0x3f));
            } else if (0x800 <= wc && wc <= 0xffff) {
                s += (0xe0 | (wc >> 12));
                s += (0x80 | ((wc >> 6) & 0x3f));
                s += (0x80 | (wc & 0x3f));
            } else if (0x10000 <= wc && wc <= 0x1fffff) {
                s += (0xf0 | (wc >> 18));
                s += (0x80 | ((wc >> 12) & 0x3f));
                s += (0x80 | ((wc >> 6) & 0x3f));
                s += (0x80 | (wc & 0x3f));
            } else if (0x200000 <= wc && wc <= 0x3ffffff) {
                s += (0xf8 | (wc >> 24));
                s += (0x80 | ((wc >> 18) & 0x3f));
                s += (0x80 | ((wc >> 12) & 0x3f));
                s += (0x80 | ((wc >> 6) & 0x3f));
                s += (0x80 | (wc & 0x3f));
            } else if (0x4000000 <= wc && wc <= 0x7fffffff) {
                s += (0xfc | (wc >> 30));
                s += (0x80 | ((wc >> 24) & 0x3f));
                s += (0x80 | ((wc >> 18) & 0x3f));
                s += (0x80 | ((wc >> 12) & 0x3f));
                s += (0x80 | ((wc >> 6) & 0x3f));
                s += (0x80 | (wc & 0x3f));
            }
        }
        return s;
    }

    template<typename T>
    std::string const& U8AsString(T const& u8s) {
        return (std::string const&)u8s;
    }

    template<typename T>
    std::string U8AsString(T&& u8s) {
        return (std::string&&)u8s;
    }


    /************************************************************************************/
    // StringFuncs 继续适配各种常见数据类型
    /************************************************************************************/

    // 适配 char* \0 结尾 字串
    template<>
    struct StringFuncs<char*, void> {
        static inline void Append(std::string& s, char* const& in) {
            s.append(in ? in: "null");
        }
    };

    // 适配 char const* \0 结尾 字串
    template<>
    struct StringFuncs<char const*, void> {
        static inline void Append(std::string& s, char const* const& in) {
            s.append(in ? in: "null");
        }
    };

    // 适配 literal char[len] string
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(in);
        }
    };

    // 适配 std::string_view
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::string_view, T> || std::is_base_of_v<std::u8string_view, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append((std::string_view&)in);
        }
    };

    // 适配 std::u32string_view
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::u32string_view, T>>> {
        static inline void Append(std::string& s, T const& in) {
            for (auto& c : in) {
                Append(s, c);
            }
        }
    };

    // 适配 std::string( 前后加引号 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<std::string, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('\"');
            s.append(in);
            s.push_back('\"');
        }
    };

    // 适配 type_info     typeid(T)
    template<>
    struct StringFuncs<std::type_info, void> {
        static inline void Append(std::string& s, std::type_info const& in) {
            s.push_back('\"');
            s.append(in.name());
            s.push_back('\"');
        }
    };

    // 用来方便的插入空格
    struct CharRepeater {
        char item;
        size_t len;
    };
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<CharRepeater, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(in.len, in.item);
        }
    };


    // 适配所有数字( char32_t 会转为 utf8 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            if constexpr (std::is_same_v<bool, std::decay_t<T>>) {
                s.append(in ? "true" : "false");
            }
            else if constexpr (std::is_same_v<char, std::decay_t<T>> || std::is_same_v<char8_t, std::decay_t<T>>) {
                s.push_back((char)in);
            }
            else if constexpr (std::is_floating_point_v<T>) {
                std::array<char, 40> buf;
#ifndef _MSC_VER
                snprintf(buf.data(), buf.size(), "%.16lf", (double) in);
                s.append(buf.data());
#else
                auto [ptr, _] = std::to_chars(buf.data(), buf.data() + buf.size(), in, std::chars_format::general, 16);
                s.append(std::string_view(buf.data(), ptr - buf.data()));
#endif
            }
            else {
                if constexpr (std::is_same_v<char32_t, std::decay_t<T>>) {
                    char buf[8];
                    auto siz = Char32ToUtf8(in, buf);
                    s.append(std::string_view(buf, siz));
                }
                else {
                    //s.append(std::to_string(in));
                    std::array<char, 40> buf;
                    auto [ptr, _] = std::to_chars(buf.data(), buf.data() + buf.size(), in);
                    s.append(std::string_view(buf.data(), ptr - buf.data()));
                }
            }
        }
    };

    // 适配 enum( 根据原始数据类型调上面的适配 )
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.append(std::to_string((std::underlying_type_t<T>)in));
        }
    };

    // 适配 TimePoint
    template<typename C, typename D>
    struct StringFuncs<std::chrono::time_point<C, D>, void> {
        static inline void Append(std::string& s, std::chrono::time_point<C, D> const& in) {
            AppendTimePoint_Local(s, in);
        }
    };

    // 适配 std::optional<T>
    template<typename T>
    struct StringFuncs<std::optional<T>, void> {
        static inline void Append(std::string &s, std::optional<T> const &in) {
            if (in.has_value()) {
                ::xx::Append(s, in.value());
            } else {
                s.append("null");
            }
        }
    };

    // 适配 std::vector<T>
    template<typename T>
    struct StringFuncs<std::vector<T>, void> {
        static inline void Append(std::string& s, std::vector<T> const& in) {
            s.push_back('[');
            if (auto inLen = in.size()) {
                for(size_t i = 0; i < inLen; ++i) {
                    ::xx::Append(s, in[i]);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::unordered_set<T>
    template<typename T>
    struct StringFuncs<std::unordered_set<T>, void> {
        static inline void Append(std::string& s, std::unordered_set<T> const& in) {
            s.push_back('[');
            if (!in.empty()) {
                for(auto&& o : in) {
                    ::xx::Append(s, o);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::unordered_map<K, V>
    template<typename K, typename V>
    struct StringFuncs<std::unordered_map<K, V>, void> {
        static inline void Append(std::string& s, std::unordered_map<K, V> const& in) {
            s.push_back('[');
            if (!in.empty()) {
                for (auto &kv : in) {
                    ::xx::Append(s, kv.first);
                    s.push_back(',');
                    ::xx::Append(s, kv.second);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::map<K, V>
    template<typename K, typename V>
    struct StringFuncs<std::map<K, V>, void> {
        static inline void Append(std::string& s, std::map<K, V> const& in) {
            s.push_back('[');
            if (!in.empty()) {
                for (auto &kv : in) {
                    ::xx::Append(s, kv.first);
                    s.push_back(',');
                    ::xx::Append(s, kv.second);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };

    // 适配 std::pair<K, V>
    template<typename K, typename V>
    struct StringFuncs<std::pair<K, V>, void> {
        static inline void Append(std::string& s, std::pair<K, V> const& in) {
            s.push_back('[');
            ::xx::Append(s, in.first);
            s.push_back(',');
            ::xx::Append(s, in.second);
            s.push_back(']');
        }
    };
	
	
    // 适配 std::tuple<......>
    template<typename...T>
    struct StringFuncs<std::tuple<T...>, void> {
        static inline void Append(std::string &s, std::tuple<T...> const &in) {
            s.push_back('[');
            std::apply([&](auto const &... args) {
                (::xx::Append(s, args, ','), ...);
                if constexpr(sizeof...(args) > 0) {
                    s.resize(s.size() - 1);
                }
            }, in);
            s.push_back(']');
        }
    };

    // 适配 std::variant<......>
    template<typename...T>
    struct StringFuncs<std::variant<T...>, void> {
        static inline void Append(std::string& s, std::variant<T...> const& in) {
            std::visit([&](auto const& v) {
                ::xx::Append(s, v);
            }, in);
        }
    };

    // 适配 Span, Data_r, Data_rw
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<Span, T>>> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('[');
            if (auto inLen = in.len) {
                for (size_t i = 0; i < inLen; ++i) {
                    ::xx::Append(s, (uint8_t)in[i]);
                    s.push_back(',');
                }
                s[s.size() - 1] = ']';
            }
            else {
                s.push_back(']');
            }
        }
    };


    /************************************************************************************/
    // utils
    /************************************************************************************/

    constexpr std::string_view base64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"sv;

    inline std::string Base64Encode(std::string_view const& in) {
        std::string out;
        int val = 0, valb = -6;
        for (uint8_t c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(base64chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(base64chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    inline std::string Base64Decode(std::string_view const& in) {
        std::string out;
        std::array<int, 256> T;
        T.fill(-1);
        for (int i = 0; i < 64; i++) T[base64chars[i]] = i;
        int val = 0, valb = -8;
        for (uint8_t c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }


    inline constexpr std::string_view TrimRight(std::string_view const& s) {
        auto idx = s.find_last_not_of(" \t\n\r\f\v");
        if (idx == std::string_view::npos) return { s.data(), 0 };
        return { s.data(), idx + 1 };
    }

    inline constexpr std::string_view TrimLeft(std::string_view const& s) {
        auto idx = s.find_first_not_of(" \t\n\r\f\v");
        if (idx == std::string_view::npos) return { s.data(), 0 };
        return { s.data() + idx, s.size() - idx };
    }

    inline constexpr std::string_view Trim(std::string_view const& s) {
        return TrimLeft(TrimRight(s));
    }

    template<size_t numDelimiters>
    inline constexpr std::string_view SplitOnce(std::string_view& sv, char const(&delimiters)[numDelimiters]) {
        static_assert(numDelimiters >= 2);
        auto data = sv.data();
        auto siz = sv.size();
        for (size_t i = 0; i != siz; ++i) {
            bool found;
            if constexpr (numDelimiters == 2) {
                found = sv[i] == delimiters[0];
            }
            else {
                found = std::string_view(delimiters).find(sv[i]) != std::string_view::npos;
            }
            if (found) {
                sv = std::string_view(data + i + 1, siz - i - 1);
                return {data, i};
            }
        }
        sv = std::string_view(data + siz, 0);
        return {data, siz};
    }

    template<typename T>
    inline constexpr bool SvToNumber(std::string_view const& input, T& out) {
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range;
    }

    template<typename T>
    inline constexpr T SvToNumber(std::string_view const& input, T const& defaultValue) {
        T out;
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range ? out : defaultValue;
    }

    template<typename T>
    inline constexpr std::optional<T> SvToNumber(std::string_view const& input) {
        T out;
        auto&& r = std::from_chars(input.data(), input.data() + input.size(), out);
        return r.ec != std::errc::invalid_argument && r.ec != std::errc::result_out_of_range ? out : std::optional<T>{};
    }

    template<typename T>
    inline std::string_view ToStringView(T const& v, char* const& buf, size_t const& len) {
        static_assert(std::is_integral_v<T>);
        if (auto [ptr, ec] = std::to_chars(buf, buf + len, v); ec == std::errc()) {
            return { buf, size_t(ptr - buf) };
        }
        else return {};
    }

    template<typename T, size_t len>
    inline std::string_view ToStringView(T const& v, char(&buf)[len]) {
        return ToStringView<T>(v, buf, len);
    }

    // 转换 s 数据类型 为 T 填充 dst
    template<typename T>
    inline void Convert(char const* const& s, T& dst) {
        if (!s) {
            dst = T();
        }
        else if constexpr (std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) <= 4) {
            dst = (T)strtoul(s, nullptr, 0);
        }
        else if constexpr (std::is_integral_v<T> && !std::is_unsigned_v<T> && sizeof(T) <= 4) {
            dst = (T)atoi(s);
        }
        else if constexpr (std::is_integral_v<T>&& std::is_unsigned_v<T> && sizeof(T) == 8) {
            dst = strtoull(s, nullptr, 0);
        }
        else if constexpr (std::is_integral_v<T> && !std::is_unsigned_v<T> && sizeof(T) == 8) {
            dst = atoll(s);
        }
        else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) {
            dst = strtof(s, nullptr);
        }
        else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) {
            dst = atof(s);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            dst = s[0] == '1' || s[0] == 't' || s[0] == 'T' || s[0] == 'y' || s[0] == 'Y';
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            dst = s;
        }
        // todo: more
    }


    inline int FromHex(uint8_t const& c) {
        if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
        else if (c >= 'a' && c <= 'z') return c - 'a' + 10;
        else if (c >= '0' && c <= '9') return c - '0';
        else return 0;
    }
    inline uint8_t FromHex(char const* c) {
        return ((uint8_t)FromHex(c[0]) << 4) | (uint8_t)FromHex(c[1]);
    }

    inline void ToHex(uint8_t const& c, uint8_t& h1, uint8_t& h2) {
        auto a = c / 16;
        auto b = c % 16;
        h1 = (uint8_t)(a + ((a <= 9) ? '0' : ('a' - 10)));
        h2 = (uint8_t)(b + ((b <= 9) ? '0' : ('a' - 10)));
    }

    inline void ToHex(std::string& s) {
        auto len = s.size();
        s.resize(len * 2);
        auto b = (uint8_t*)s.data();
        for (auto i = (ptrdiff_t)len - 1; i >= 0; --i) {
            xx::ToHex(b[i], b[i * 2], b[i * 2 + 1]);
        }
    }



    // 用 s 滚动异或 buf 内容. 注意传入 buf 需要字节对齐, 小尾适用
    inline void XorContent(uint64_t s, char* buf, size_t len) {
        auto p = (char*)&s;
        auto left = len % sizeof(s);                                                                        // 余数 ( 这里假定 buf 一定会按 4/8 字节对齐, 小尾 )
        size_t i = 0;
        for (; i < len - left; i += sizeof(s)) {                                                            // 把字节对齐的部分肏了
            *(uint64_t*)&buf[i] ^= s;
        }
        for (auto j = i; i < len; ++i) {                                                                    // 余下部分单字节肏
            buf[i] ^= p[i - j];
        }
    }

    // 用 s 滚动异或 b 内容
    inline void XorContent(char const* s, size_t slen, char* b, size_t blen) {
        auto e = b + blen;
        for (size_t i = 0; b < e; *b++ ^= s[i++]) {
            if (i == slen) i = 0;
        }
    }




    // 移除文件路径部分只剩文件名
    inline int RemovePath(std::string& s) {
        auto b = s.data();
        auto e = (int)s.size() - 1;
        for (int i = e; i >= 0; --i) {
            if (b[i] == '/' || b[i] == '\\') {
                memmove(b, b + i + 1, e - i);
                s.resize(e - i);
                return i + 1;
            }
        }
        return 0;
    }
	
    // 获取 1, 2 级文件扩展名
    inline std::pair<std::string_view, std::string_view> GetFileNameExts(std::string const& fn) {
        std::pair<std::string_view, std::string_view> rtv;
        auto dotPos = fn.rfind('.');
        auto extLen = fn.size() - dotPos;
        rtv.first = std::string_view(fn.data() + dotPos, extLen);
        if (dotPos) {
            dotPos = fn.rfind('.', dotPos - 1);
            if(dotPos != std::string::npos) {
                extLen = fn.size() - dotPos - extLen;
                rtv.second = std::string_view(fn.data() + dotPos, extLen);
            }
        }
        return rtv;
    }


    // 将 string 里数字部分转为 n 字节定长（前面补0）后返回( 方便排序 ). 不支持小数
    inline std::string InnerNumberToFixed(std::string_view const& s, int const& n = 16) {
        std::string t, d;
        bool handleDigit = false;
        for (auto&& c : s) {
            if (c >= '0' && c <= '9') {
                if (!handleDigit) {
                    handleDigit = true;
                }
                d.append(1, c);
            }
            else {
                if (handleDigit) {
                    handleDigit = false;
                    t.append(n - d.size(), '0');
                    t.append(d);
                    d.clear();
                }
                else {
                    t.append(1, c);
                }
            }
        }
        if (handleDigit) {
            handleDigit = false;
            t.append(n - d.size(), '0');
            t.append(d);
            d.clear();
        }
        return t;
    }



    /************************************************************************************/
    // 各种 Cout
    /************************************************************************************/

    // 替代 std::cout. 支持实现了 StringFuncs 模板适配的类型
    template<typename...Args>
    inline void Cout(Args const& ...args) {
        std::string s;
        Append(s, args...);
        for (auto&& c : s) {
            if (!c) c = '^';
        }
        std::cout << s;
    }

    // 在 Cout 基础上添加了换行
    template<typename...Args>
    inline void CoutN(Args const& ...args) {
        Cout(args...);
        std::cout << std::endl;
    }

    // 在 CoutN 基础上于头部添加了时间
    template<typename...Args>
    inline void CoutTN(Args const& ...args) {
        CoutN("[", std::chrono::system_clock::now(), "] ", args...);
    }

    // 立刻输出
    inline void CoutFlush() {
        std::cout.flush();
    }

    // 带 format 格式化的 Cout
    template<typename...Args>
    inline void CoutFormat(char const* const& format, Args const& ...args) {
        std::string s;
        AppendFormat(s, format, args...);
        for (auto&& c : s) {
            if (!c) c = '^';
        }
        std::cout << s;
    }

    // 在 CoutFormat 基础上添加了换行
    template<typename...Args>
    inline void CoutNFormat(char const* const& format, Args const& ...args) {
        CoutFormat(format, args...);
        std::cout << std::endl;
    }

    // 在 CoutNFormat 基础上于头部添加了时间
    template<typename...Args>
    inline void CoutTNFormat(char const* const& format, Args const& ...args) {
        CoutNFormat("[", std::chrono::system_clock::now(), "] ", args...);
    }
}
