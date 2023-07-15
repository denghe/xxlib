#pragma once

#include <bit>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <chrono>
#include <optional>
#include <variant>
#include <array>
#include <tuple>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <sstream>
#include <string_view>
#include <charconv>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <memory>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <filesystem>
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
#else
static_assert(false, "No co_await support");
#endif
#if __has_include(<span>)
#include <span>
#endif
#if __has_include(<format>)
#include <format>
#endif

#define _USE_MATH_DEFINES  // needed for M_PI and M_PI2
#include <math.h>          // M_PI
#undef _USE_MATH_DEFINES
#include <cstddef>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>


#ifdef _WIN32
#ifndef NOMINMAX
#	define NOMINMAX
#endif
#	define NODRAWTEXT
//#	define NOGDI            // d3d9 need it
#	define NOBITMAP
#	define NOMCX
#	define NOSERVICE
#	define NOHELP
#	define WIN32_LEAN_AND_MEAN
#   include <WinSock2.h>
#   include <process.h>
#	include <Windows.h>
#   include <intrin.h>     // _BitScanReverseXXXX _byteswap_XXXX
#   include <ShlObj.h>
#else
#	include <unistd.h>    // for usleep
#   include <arpa/inet.h>  // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN
#endif

#ifndef XX_NOINLINE
#   ifndef NDEBUG
#       define XX_NOINLINE
#       define XX_FORCE_INLINE
#       define XX_INLINE
#   else
#       ifdef _MSC_VER
#           define XX_NOINLINE __declspec(noinline)
#           define XX_FORCE_INLINE __forceinline
#           define XX_INLINE __forceinline
#       else
#           define XX_NOINLINE __attribute__((noinline))
#           define XX_FORCE_INLINE __attribute__((always_inline))
#           define XX_INLINE __attribute__((always_inline))
#       endif
#   endif
#endif

#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define __LITTLE_ENDIAN__
#   elif __BYTE_ORDER == __BIG_ENDIAN
#       define __BIG_ENDIAN__
#   elif _WIN32
#       define __LITTLE_ENDIAN__
#   endif
#endif

#ifndef XX_STRINGIFY
#	define XX_STRINGIFY(x)  XX_STRINGIFY_(x)
#	define XX_STRINGIFY_(x)  #x
#endif

#if defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL) || defined(__x86_64__) || defined(_M_X64)
#    define XX_ARCH_IA
#endif

#if defined(__LP64__) || __WORDSIZE == 64 || defined(_WIN64) || defined(_M_X64)
#    define XX_ARCH_64
#endif

#ifdef _MSC_VER
#    define XX_ALIGN2( x )		    __declspec(align(2)) x
#    define XX_ALIGN4( x )		    __declspec(align(4)) x
#    define XX_ALIGN8( x )		    __declspec(align(8)) x
#    define XX_ALIGN16( x )		    __declspec(align(16)) x
#    define XX_ALIGN32( x )		    __declspec(align(32)) x
#    define XX_ALIGN64( x )		    __declspec(align(64)) x
#else
#    define XX_ALIGN2( x )          x __attribute__ ((aligned (2)))
#    define XX_ALIGN4( x )          x __attribute__ ((aligned (4)))
#    define XX_ALIGN8( x )          x __attribute__ ((aligned (8)))
#    define XX_ALIGN16( x )         x __attribute__ ((aligned (16)))
#    define XX_ALIGN32( x )         x __attribute__ ((aligned (32)))
#    define XX_ALIGN64( x )         x __attribute__ ((aligned (64)))
#endif

#ifdef _MSC_VER
#    define XX_LIKELY(x)                 (x)
#    define XX_UNLIKELY(x)               (x)
#else
#    define XX_UNLIKELY(x)               __builtin_expect((x), 0)
#    define XX_LIKELY(x)                 __builtin_expect((x), 1)
#endif

// __restrict like?
#if defined(__clang__)
#  define XX_ASSUME(e) __builtin_assume(e)
#elif defined(__GNUC__) && !defined(__ICC)
#  define XX_ASSUME(e) if (e) {} else { __builtin_unreachable(); }  // waiting for gcc13 c++23 [[assume]]
#elif defined(_MSC_VER) || defined(__ICC)
#  define XX_ASSUME(e) __assume(e)
#endif

#ifndef _countof
template<typename T, size_t N>
size_t _countof(T const (&arr)[N]) {
    return N;
}
#endif

#ifndef _offsetof
#define _offsetof(s,m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - _offsetof(type, member)))
#endif

#ifndef _WIN32
inline void Sleep(int const& ms) {
    usleep(ms * 1000);
}
#endif

#ifdef XX_DISABLE_ASSERT_IN_RELEASE
#include <assert.h>
#define xx_assert assert
#else

#ifdef NDEBUG
#undef NDEBUG
#define HAS_NDEBUG
#endif
#include <assert.h>
#ifdef HAS_NDEBUG
#define NDEBUG
#endif
#ifdef _MSC_VER
#define xx_assert(expression) (void)(                                                        \
            (!!(expression)) ||                                                              \
            (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
        )
#elif defined(__GNUC__) and defined(__MINGW32__)
#if defined(_UNICODE) || defined(UNICODE)
#define xx_assert(_Expression) \
 (void) \
 ((!!(_Expression)) || \
  (_wassert(_CRT_WIDE(#_Expression),_CRT_WIDE(__FILE__),__LINE__),0))
#else /* not unicode */
#define xx_assert(_Expression) \
 (void) \
 ((!!(_Expression)) || \
  (_assert(#_Expression,__FILE__,__LINE__),0))
#endif /* _UNICODE||UNICODE */
#else
#ifdef EMSCRIPTEN
#define xx_assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__),0)))
#else
#define xx_assert(expression) (void)(                                                        \
            (!!(expression)) ||                                                              \
            (__assert_fail(#expression, __FILE__, __LINE__, __ASSERT_FUNCTION), 0)           \
        )
#endif
#endif
#endif

#define XX_SIMPLE_STRUCT_DEFAULT_CODES(T)\
T() = default;\
T(T const&) = default;\
T(T &&) = default;\
T& operator=(T const&) = default;\
T& operator=(T &&) = default;

// stackless 协程相关
// 当前主要用到这些宏。只有 lineNumber 一个特殊变量名要求
#define COR_BEGIN	switch (lineNumber) { case 0:
#define COR_YIELD	return __LINE__; case __LINE__:;
#define COR_EXIT	return 0;
#define COR_END		} return 0;

/*
    int lineNumber = 0;
    int Update() {
        COR_BEGIN
            // COR_YIELD
        COR_END
    }
    ... lineNumber = Update();
*/

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;


#ifdef __GNUC__
//#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
