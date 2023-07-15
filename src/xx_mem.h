#pragma once
#include "xx_includes.h"

namespace xx {
    /************************************************************************************/
    // Scope Guard( F == lambda )

    template<class F>
    [[nodiscard]] auto MakeScopeGuard(F&& f) noexcept {
        struct ScopeGuard {
            F f;
            bool cancel;

            explicit ScopeGuard(F&& f) noexcept : f(std::forward<F>(f)), cancel(false) {}

            ~ScopeGuard() noexcept { if (!cancel) { f(); } }

            inline void Cancel() noexcept { cancel = true; }

            inline void operator()(bool cancel = false) {
                f();
                this->cancel = cancel;
            }
        };
        return ScopeGuard(std::forward<F>(f));
    }


    template<class F>
    [[nodiscard]] auto MakeSimpleScopeGuard(F&& f) noexcept {
        struct SG { F f; SG(F&& f) noexcept : f(std::forward<F>(f)) {} ~SG() { f(); } };
        return SG(std::forward<F>(f));
    }


    /************************************************************************************/
    // mem utils

    // 数字字节序交换
    template<typename T>
    static T BSwap(T const& i) {
        T r;
#ifdef _WIN32
        if constexpr (sizeof(T) == 2) *(uint16_t*)&r = _byteswap_ushort(*(uint16_t*)&i);
        if constexpr (sizeof(T) == 4) *(uint32_t*)&r = _byteswap_ulong(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) *(uint64_t*)&r = _byteswap_uint64(*(uint64_t*)&i);
#else
        if constexpr (sizeof(T) == 2) *(uint16_t*)&r = __builtin_bswap16(*(uint16_t*)&i);
        if constexpr (sizeof(T) == 4) *(uint32_t*)&r = __builtin_bswap32(*(uint32_t*)&i);
        if constexpr (sizeof(T) == 8) *(uint64_t*)&r = __builtin_bswap64(*(uint64_t*)&i);
#endif
        return r;
    }

    // 带符号整数 解码 return (in 为单数) ? -(in + 1) / 2 : in / 2
    [[maybe_unused]] static int16_t ZigZagDecode(uint16_t const& in) {
        return (int16_t)((int16_t)(in >> 1) ^ (-(int16_t)(in & 1)));
    }
    [[maybe_unused]] static int32_t ZigZagDecode(uint32_t const& in) {
        return (int32_t)(in >> 1) ^ (-(int32_t)(in & 1));
    }
    [[maybe_unused]] static int64_t ZigZagDecode(uint64_t const& in) {
        return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
    }

    // 返回首个出现 1 的 bit 的下标
    static size_t Calc2n(size_t const& n) {
        assert(n);
#ifdef _MSC_VER
        unsigned long r = 0;
# if defined(_WIN64) || defined(_M_X64)
        _BitScanReverse64(&r, n);
# else
        _BitScanReverse(&r, n);
# endif
        return (std::size_t)r;
#else
# if defined(__LP64__) || __WORDSIZE == 64
        return int(63 - __builtin_clzl(n));
# else
        return int(31 - __builtin_clz(n));
# endif
#endif
    }

    // 返回一个刚好大于 n 的 2^x 对齐数
    static size_t Round2n(size_t const& n) {
        auto rtv = size_t(1) << Calc2n(n);
        if (rtv == n) return n;
        else return rtv << 1;
    }

    // 带符号整数 编码  return in < 0 ? (-in * 2 - 1) : (in * 2)
    [[maybe_unused]] static uint16_t ZigZagEncode(int16_t const& in) {
        return (uint16_t)((in << 1) ^ (in >> 15));
    }
    [[maybe_unused]] static uint32_t ZigZagEncode(int32_t const& in) {
        return (in << 1) ^ (in >> 31);
    }
    [[maybe_unused]] static uint64_t ZigZagEncode(int64_t const& in) {
        return (in << 1) ^ (in >> 63);
    }
}
