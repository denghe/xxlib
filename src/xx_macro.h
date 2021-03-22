#pragma once

#ifndef _WIN32
#include <arpa/inet.h>  // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN
#endif

#ifndef XX_NOINLINE
#   ifndef NDEBUG
#       define XX_NOINLINE
#       define XX_FORCE_INLINE
#   else
#       ifdef _MSC_VER
#           define XX_NOINLINE __declspec(noinline)
#           define XX_FORCE_INLINE __forceinline
#       else
#           define XX_NOINLINE __attribute__((noinline))
#           define XX_FORCE_INLINE __attribute__((always_inline))
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

#define XX_HAS_TYPEDEF( TN ) \
template<typename, typename = void> struct HasTypedef_##TN : std::false_type {}; \
template<typename T> struct HasTypedef_##TN<T, std::void_t<typename T::TN>> : std::true_type {}; \
template<typename T> constexpr bool TN = HasTypedef_##TN<T>::value;
