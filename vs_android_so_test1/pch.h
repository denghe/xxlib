#pragma once
#ifdef __ANDROID__
#include <jni.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "vs_android_so_test1", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "vs_android_so_test1", __VA_ARGS__))
#endif

#ifdef WIN32DLL_EXPORTS
#  define DLLEXPORT __declspec(dllexport)
#else
#  define DLLEXPORT
#endif

#include <xx_asio_tcp_client_cpp.h>

// F = function
// I = int, U = uint, L = int64, UL = uint64
// D = double, F = float, S = char*, V = void

typedef int(*FI)();
typedef void(*FVI)(int);
typedef void(*FVIF)(int, float);
typedef void(*FVIFF)(int, float, float);
typedef void(*FVIFFF)(int, float, float, float);
// ...

struct Externs {
	inline static FI SpriteNew = nullptr;
	inline static FVI SpriteDelete = nullptr;
	inline static FVIFF SpritePos = nullptr;
	// ...

	inline static bool AllAssigned() {
		return SpriteNew
			&& SpriteDelete
			&& SpritePos
			// ...
			;
	}
};

struct Logic;

extern "C" {
	DLLEXPORT inline void SetFunc_SpriteNew(FI f) { Externs::SpriteNew = f; }
	DLLEXPORT inline void SetFunc_DeleteSprite(FVI f) { Externs::SpriteDelete = f; }
	DLLEXPORT inline void SetFunc_SpritePos(FVIFF f) { Externs::SpritePos = f; }
	// ...

	DLLEXPORT Logic* LogicNew();
	DLLEXPORT void LogicUpdate(Logic * self, float delta);
	DLLEXPORT void LogicDelete(Logic * self);
}

#define STRUCT_BASE_CODE_CORE(T) T()=default;T(T const&)=delete;T& operator=(T const&)=delete;
#define STRUCT_BASE_CODE(T) T(T&&)=default;T& operator=(T&&)=default;STRUCT_BASE_CODE_CORE(T)

