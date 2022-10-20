#pragma once
#include <xx_string.h>
#include <xx_data.h>

namespace xx {
    /************************************************************************************/
    // headers & args
    /************************************************************************************/

    // string_view 键值对 数组容器, 常用于保存 headers, arguments 带 value 类型转换
    template<size_t len>
    struct SVPairArray : std::array<std::pair<std::string_view, std::string_view>, len> {
        using BaseType = std::array<std::pair<std::string_view, std::string_view>, len>;
        using BaseType::BaseType;
        using BaseType::operator[];

        // 用 key 查 value. 找不到返回 {}
        std::string_view operator[](std::string_view const& key) {
            for (size_t i = 0; i < len; i++) {
                if ((*this)[i].first == key) return (*this)[i].second;
            }
            return {};
        }

        // for ToTuple
        template<size_t I = 0, typename T>
        void FillTuple(T& t) {
            using ET = std::tuple_element_t<I, T>;
            if constexpr (std::is_same_v<std::string_view, ET>) {
                std::get<I>(t) = (*this)[I].second;
            }
            else if constexpr (IsOptional_v< ET >) {
                std::get<I>(t) = SvToNumber< typename ET::value_type >((*this)[I].second);
            }
            else {
                std::get<I>(t) = SvToNumber< ET >((*this)[I].second, ET{});
            }
            if constexpr (I + 1 != std::tuple_size_v<T>) {
                FillTuple<I + 1>(t);
            }
        }

        // 将 array 的 value 转储到 tuple. 类型只能是 std::string_view 或 number 或 std::optional< number >
        template<typename...TS>
        std::tuple<TS...> ToTuple() {
            std::tuple<TS...> t;
            FillTuple(t);
            return t;
        }
    };


    /************************************************************************************/
    // url
    /************************************************************************************/

    // 将解码后的 url 覆盖填充到 dst
    inline void UrlDecode(std::string const& src, std::string& dst) {
        auto&& length = src.size();
        dst.clear();
        dst.reserve(length);
        for (size_t i = 0; i < length; i++) {
            if (src[i] == '+') {
                dst += ' ';
            }
            else if (src[i] == '%') {
                if (i + 2 >= length) return;
                auto high = FromHex(src[i + 1]);
                auto low = FromHex(src[i + 2]);
                i += 2;
                dst += ((char)(uint8_t)(high * 16 + low));
            }
            else dst += src[i];
        }
    }

    // 上面函数的返回值版
    inline std::string UrlDecode(std::string const& src) {
        std::string rtv;
        ::xx::UrlDecode(src, rtv);
        return rtv;
    }

    // 将编码后的 url 覆盖填充到 dst
    inline void UrlEncode(std::string const& src, std::string& dst) {
        auto&& str = src.c_str();
        auto&& siz = src.size();
        dst.clear();
        dst.reserve(siz * 2);
        for (size_t i = 0; i < siz; ++i) {
            char c = str[i];
            if ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||
                c == '*' || c == '\'' || c == '(' || c == ')') {
                dst += c;
            }
            else if (c == ' ') {
                dst += '+';
            }
            else
            {
                dst += '%';
                uint8_t h1, h2;
                ToHex(c, h1, h2);
                dst += h1;
                dst += h2;
            }
        }
    }

    // 上面函数的返回值版
    inline std::string UrlEncode(std::string const& src) {
        std::string rtv;
        ::xx::UrlEncode(src, rtv);
        return rtv;
    }

    /************************************************************************************/
    // html
    /************************************************************************************/

    enum class HttpEncodeTypes {
        Html,	// & < > ' " 
        Text,	// & < > space
        Pre		// & < >
    };

    constexpr const unsigned char CharEscapeTypes[] = {
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 4, 9, 9, 9, 0, 3, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 9, 1, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    };

    constexpr const unsigned char CharEscapeLens[] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 6, 1, 1, 1, 5, 6, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 4, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };

    template<HttpEncodeTypes t>
    XX_FORCE_INLINE void HttpEncodeTo_(char* const& buf, size_t& len, char const& c) {
        switch (CharEscapeTypes[(size_t)c]) {
        case 0:
            memcpy(buf + len, "&amp;", 5);
            len += 5;
            break;
        case 1:
            memcpy(buf + len, "&gt;", 4);
            len += 4;
            break;
        case 2:
            memcpy(buf + len, "&lt;", 4);
            len += 4;
            break;
        case 3:
            if constexpr (t == HttpEncodeTypes::Html) {
                memcpy(buf + len, "&apos;", 6);
                len += 6;
                break;
            }
        case 4:
            if constexpr (t == HttpEncodeTypes::Html) {
                memcpy(buf + len, "&quot;", 6);
                len += 6;
                break;
            }
        default:
            buf[len++] = c;
            break;
        }
    }

    template<HttpEncodeTypes t>
    XX_FORCE_INLINE void HttpEncodeTo_(char* const& buf, size_t& len, std::string_view const& s) {
        for (auto& c : s) {
            HttpEncodeTo_<t>(buf, len, c);
        }
    }

    template<HttpEncodeTypes t>
    XX_FORCE_INLINE void HttpEncodeTo(Data& d, std::string_view const& s) {
        d.Reserve(d.len + s.size() * 5);
        HttpEncodeTo_<t>((char*)d.buf, d.len, s);
    }

    template<HttpEncodeTypes t>
    XX_FORCE_INLINE void HttpEncodeTo(std::string& d, std::string_view const& s) {
        auto len = d.size();
        size_t cap = len;
        for (auto& c : s) {
            cap += CharEscapeLens[(size_t)c];
        }
        d.resize(cap);
        HttpEncodeTo_<t>(d.data(), len, s);
    }

    // T 通常为 xx::Data, std::string, std::vector<char> 啥的
    template<typename T>
    void HttpEncodeHtmlTo(T& d, std::string_view const& s) {
        HttpEncodeTo<HttpEncodeTypes::Html>(d, s);
    }
    template<typename T>
    void HttpEncodeTextTo(T& d, std::string_view const& s) {
        HttpEncodeTo<HttpEncodeTypes::Text>(d, s);
    }
    template<typename T>
    void HttpEncodePreTo(T& d, std::string_view const& s) {
        HttpEncodeTo<HttpEncodeTypes::Pre>(d, s);
    }

    // 上面函数的 std::string 返回值版
    inline std::string HttpEncodeHtml(std::string_view const& s) {
        std::string r;
        ::xx::HttpEncodeHtmlTo(r, s);
        return r;
    }
    inline std::string HttpEncodeText(std::string_view const& s) {
        std::string r;
        ::xx::HttpEncodeTextTo(r, s);
        return r;
    }
    inline std::string HttpEncodePre(std::string_view const& s) {
        std::string r;
        ::xx::HttpEncodePreTo(r, s);
        return r;
    }
}
