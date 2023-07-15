#pragma once
#include "xx_includes.h"
#include "xx_typetraits.h"
#include "xx_hash.h"
#include "xx_time.h"
#include "xx_mem.h"
#include "xx_data.h"
#include "xx_string.h"
#include "xx_list.h"

#if __has_include(<xx_hide_string.h>)
# include <xx_hide_string.h>
#endif

namespace xx {

#ifdef _WIN32
	template<bool hideWindow = true>
	inline static HANDLE PopupConsole(LPCSTR exePath, LPSTR cmdLineArgs) {
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		if constexpr (hideWindow) {
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
		}
		if (!CreateProcessA(
			exePath,
			cmdLineArgs,
			NULL,
			NULL,
			FALSE,
			CREATE_NEW_CONSOLE,
			NULL,
			NULL,
			&si,
			&pi
		)) {
			auto e = GetLastError();
			CloseHandle(pi.hThread);
			return 0;
		}
		return pi.hProcess;
	}
	/*
	if (h != NULL) {
		while (WAIT_TIMEOUT == WaitForSingleObject(ff, 500)) {
			// your wait code goes here.
		}
		CloseHandle(h);
	}
	*/
#endif


	template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
	inline bool EnumContains(E const& a, E const& b) {
		using U = std::underlying_type_t<E>;
		return U(a) & U(b);
	}
}

// 可为 enum 类型附加一些可能用得到的 运算符操作
#define XX_ENUM_OPERATOR_EXT( EnumTypeName )                                                                    \
inline EnumTypeName operator|(EnumTypeName const& a, EnumTypeName const& b) {									\
	return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) | (std::underlying_type_t<EnumTypeName>)(b)); \
}																												\
inline EnumTypeName operator+(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)	{			\
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + b);											\
}                                                                                                               \
inline EnumTypeName operator+(EnumTypeName const &a, EnumTypeName const &b) {                                   \
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + (std::underlying_type_t<EnumTypeName>)(b));	\
}                                                                                                               \
inline EnumTypeName operator-(EnumTypeName const &a, std::underlying_type_t<EnumTypeName> const &b)	{			\
    return EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) - b);											\
}                                                                                                               \
inline std::underlying_type_t<EnumTypeName> operator-(EnumTypeName const &a, EnumTypeName const &b)	{			\
    return (std::underlying_type_t<EnumTypeName>)(a) - (std::underlying_type_t<EnumTypeName>)(b);				\
}                                                                                                               \
inline EnumTypeName operator++(EnumTypeName &a) {                                                               \
    a = EnumTypeName((std::underlying_type_t<EnumTypeName>)(a) + 1);											\
    return a;                                                                                                   \
}
