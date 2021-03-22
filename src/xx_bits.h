#pragma once

#include "xx_macro.h"
#include <cstdint>
#include <cassert>

#ifdef _WIN32
#include <intrin.h>     // _BitScanReverse[64] _byteswap_ulong _byteswap_uint64
//#include <objbase.h>
#endif

namespace xx {
    // 数字字节序交换
    template<typename T>
    XX_FORCE_INLINE static T BSwap(T const& i) {
        if constexpr (sizeof(T) == 2) return (*(uint16_t*)&i >> 8) | (*(uint16_t*)&i << 8);
#ifdef _WIN32
        if constexpr (sizeof(T) == 4) return (T)_byteswap_ulong(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) return (T)_byteswap_uint64(*(uint64_t*)&i);
#else
        if constexpr (sizeof(T) == 4) return (T)__builtin_bswap32(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) return (T)__builtin_bswap64(*(uint64_t*)&i);
#endif
        return i;
    }

    // 带符号整数 解码 return (in 为单数) ? -(in + 1) / 2 : in / 2
    XX_FORCE_INLINE static int16_t ZigZagDecode(uint16_t const& in) noexcept {
        return (int16_t)((int16_t)(in >> 1) ^ (-(int16_t)(in & 1)));
    }
    XX_FORCE_INLINE static int32_t ZigZagDecode(uint32_t const& in) noexcept {
        return (int32_t)(in >> 1) ^ (-(int32_t)(in & 1));
    }
    XX_FORCE_INLINE static int64_t ZigZagDecode(uint64_t const& in) noexcept {
        return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
    }

    // 返回首个出现 1 的 bit 的下标
    XX_FORCE_INLINE static size_t Calc2n(size_t const& n) noexcept {
        assert(n);
#ifdef _WIN32
        unsigned long r = 0;
        if constexpr (sizeof(size_t) == 8) {
            _BitScanReverse64(&r, n);
        }
        else {
            _BitScanReverse(&r, (unsigned long)n);
        }
        return (size_t)r;
#else
        if constexpr (sizeof(size_t) == 8) {
            return int(63 - __builtin_clzl(n));
        }
        else {
            return int(31 - __builtin_clz(n));
        }
#endif
    }

    // 返回一个刚好大于 n 的 2^x 对齐数
    XX_FORCE_INLINE static size_t Round2n(size_t const& n) noexcept {
        auto rtv = size_t(1) << Calc2n(n);
        if (rtv == n) return n;
        else return rtv << 1;
    }

    // 带符号整数 编码  return in < 0 ? (-in * 2 - 1) : (in * 2)
    XX_FORCE_INLINE static uint16_t ZigZagEncode(int16_t const& in) noexcept {
        return (uint16_t)((in << 1) ^ (in >> 15));
    }
    XX_FORCE_INLINE static uint32_t ZigZagEncode(int32_t const& in) noexcept {
        return (in << 1) ^ (in >> 31);
    }
    XX_FORCE_INLINE static uint64_t ZigZagEncode(int64_t const& in) noexcept {
        return (in << 1) ^ (in >> 63);
    }
}
