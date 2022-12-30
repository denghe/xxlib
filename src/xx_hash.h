#pragma once
#include "xx_includes.h"
#include "xx_typetraits.h"

namespace xx {

    /***********************************************************************************/
    // Hash for Dict, tsl::xxxxxxmap. 顺序数字 key 简单绕开 std::hash
    /***********************************************************************************/

    template<typename T, typename ENABLED = void>
    struct Hash {
        inline size_t operator()(T const& k) const {
            return std::hash<T>{}(k);
        }
    };

    // 适配所有数字
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        inline size_t operator()(T const& k) const {
            return (size_t)k;
        }
    };

    // 适配 enum
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_enum_v<T>>> {
        inline size_t operator()(T const& k) const {
            return (std::underlying_type_t<T>)k;
        }
    };

    // 适配 指针( 除以 4/8 )
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_pointer_v<T>>> {
        inline size_t operator()(T const& k) const {
            if constexpr (sizeof(size_t) == 8) return (size_t)k / 8;
            else return (size_t)k / 4;
        }
    };

    // 效果: 为 key 是 string 或 string_view 的 map 提供用 string, string_view 免转换 直接对比哈希码的能力. 普通 string 后面加 sv
    // 用法: std::???????map<std::string, ????, xx::StringHasher<>, std::equal_to<void>>
    template<typename T = std::string_view, typename U = xx::Hash<T>>
    struct StringHasher {
        using is_transparent = void;
        size_t operator()(std::string_view str) const { return U{}(str); }
        size_t operator()(std::string const& str) const { return U{}(str); }
    };

    // 下面的代码方便复制使用, 以提升 hash string[_view] 性能
    /*

    #include <xxh3.h>
namespace xx {
    // 适配 std::string[_view] 走 xxhash
    template<typename T>
    struct Hash<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view>>> {
        inline size_t operator()(T const& k) const {
            return (size_t)XXH3_64bits(k.data(), k.size());
        }
    };
    // 适配 literal string
    template<typename T>
    struct Hash<T, std::enable_if_t<xx::IsLiteral_v<T>>> {
        inline size_t operator()(T const& k) const {
            return (size_t)XXH3_64bits(k, sizeof(T) - 1);
        }
    };
}

    */
}
