#pragma once
#include "xx_string.h"

namespace xx {
    // 各种 http 相关 工具函数

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

    // 将编码后的 html 覆盖填充到 dst
    inline void HtmlEncode(std::string const& src, std::string& dst) {
        auto&& str = src.c_str();
        auto&& siz = src.size();
        dst.clear();
        dst.reserve(siz * 2);	// 估算. * 6 感觉有点浪费
        for (size_t i = 0; i < siz; ++i) {
            auto c = str[i];
            switch (c) {
            case '&':  dst.append("&amp;"); break;
            case '\"': dst.append("&quot;"); break;
            case '\'': dst.append("&apos;"); break;
            case '<':  dst.append("&lt;");  break;
            case '>':  dst.append("&gt;");  break;
            default:   dst += c;			break;
            }
        }
    }

    // 上面函数的返回值版
    inline std::string HtmlEncode(std::string const& src) {
        std::string rtv;
        ::xx::HtmlEncode(src, rtv);
        return rtv;
    }

}
