﻿#pragma once

#ifndef _WIN32
#include <arpa/inet.h>  // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN
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




#if defined(__i386__) || defined(i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL) || defined(__x86_64__) || defined(_M_X64)
#    define XX_IA
#endif



#if defined(__LP64__) || __WORDSIZE == 64 || defined(_WIN64) || defined(_M_X64)
#    define XX_64
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
#    define XX_INLINE                    __forceinline
#    define XX_NOINLINE                  __declspec(noinline)
#    define XX_LIKELY(x)                 (x)
#    define XX_UNLIKELY(x)               (x)
#else
#    define XX_INLINE                    inline __attribute__((__always_inline__))
#    define XX_NOINLINE                  __attribute__((__noinline__))
#    define XX_UNLIKELY(x)               __builtin_expect((x), 0)
#    define XX_LIKELY(x)                 __builtin_expect((x), 1)
#endif





#ifndef _countof
template<typename T, size_t N>
size_t _countof(T const (&arr)[N])
{
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




#define XX_ENUM_OPERATOR_EXT( EnumTypeName )                                                                    \
inline EnumTypeName operator+(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)				\
{                                                                                                               \
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + b);											\
}                                                                                                               \
inline EnumTypeName operator+(EnumTypeName const &a, EnumTypeName const &b)                                     \
{                                                                                                               \
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + (std::underlying_type_t<EnumTypeName>)(b));	\
}                                                                                                               \
inline EnumTypeName operator-(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)				\
{                                                                                                               \
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) - b);											\
}                                                                                                               \
inline std::underlying_type_t<EnumTypeName> operator-(EnumTypeName const &a, EnumTypeName const &b)				\
{                                                                                                               \
    return (std::underlying_type_t<EnumTypeName>)(a) - (std::underlying_type_t<EnumTypeName>)(b);				\
}                                                                                                               \
inline EnumTypeName operator++(EnumTypeName &a)                                                                 \
{                                                                                                               \
    a = EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + 1);											\
    return a;                                                                                                   \
}



/*
SFINAE check menber function exists
sample：

XX_HAS_FUNC( xxxxxxxxfunc_checker, FuncName, RT ( T::* )( params... ) const );
*/
#define XX_HAS_FUNC( CN, FN, FT )   \
template<typename CT>                                                               \
class CN                                                                            \
{                                                                                   \
    template<typename T, FT> struct FuncMatcher;                                    \
    template<typename T> static char HasFunc( FuncMatcher<T, &T::FN>* );            \
    template<typename T> static int HasFunc( ... );                                 \
public:                                                                             \
    static const bool value = sizeof( HasFunc<CT>( nullptr ) ) == sizeof(char);     \
}
