#include "pch.h"

struct Sprite {
	STRUCT_BASE_CODE_CORE(Sprite);
	int _ki_ = -1;
	Sprite(Sprite&& o) { this->_ki_ = o._ki_; o._ki_ = -1; }
	Sprite& operator=(Sprite&& o) { this->_ki_ = o._ki_; o._ki_ = -1; return *this; }
	~Sprite() { Dispose(); }
	operator bool() const { return _ki_ != -1; }
	void Init() { _ki_ = Externs::SpriteNew(); }
	void Dispose() { if (_ki_ != -1) { Externs::SpriteDelete(_ki_); _ki_ = -1; } }
	void Pos(float x, float y) { Externs::SpritePos(_ki_, x, y); }
};

struct Random3 {
	STRUCT_BASE_CODE_CORE(Random3);
	uint64_t seed = 1234567891234567890;
	inline void Init(uint64_t seed_ = xx::NowSteadyEpoch10m()) { seed = seed_; }
	inline uint32_t Next() {
		seed ^= (seed << 21u);
		seed ^= (seed >> 35u);
		seed ^= (seed << 4u);
		return (uint32_t)seed;
	}
	inline int Next(int min, int max) {
		return (Next() % (max - min)) + min;
	}
};

struct Logic {
	std::vector<xx::Shared<Sprite>> sprites;
	Random3 r3;
	Logic() {
		r3.Init();
		for (size_t i = 0; i < 100; i++) {
			auto&& s = sprites.emplace_back().Emplace();
			s->Init();
			s->Pos(r3.Next(1, 1920), r3.Next(1, 1080));
		}
	}
	void Update(float delta) {
		for (auto& s : sprites) {
			s->Pos(r3.Next(1, 1920), r3.Next(1, 1080));
		}
	}
};

extern "C" {
	inline Logic* LogicNew() {
		return Externs::AllAssigned() ? new Logic() : nullptr;
	}
	inline void LogicUpdate(Logic* self, float delta) {
		if (!self) return;
		self->Update(delta);
	}
	inline void LogicDelete(Logic* self) {
		if (self) {
			delete self;
		}
	}
}


























//extern "C" {
//	/* This trivial function returns the platform ABI for which this dynamic native library is compiled.*/
//	inline const char* GetAbi()
//	{
//		// __arm__ ( __ARM_ARCH_7A__  __ARM_NEON__ )   __aarch64__    __i386__    __x86_64__
//#if defined(__arm__)
//#  if defined(__ARM_ARCH_7A__)
//#    if defined(__ARM_NEON__)
//#      define ABI "armeabi-v7a/NEON"
//#    else
//#      define ABI "armeabi-v7a"
//#    endif
//#  else
//#    define ABI "armeabi"
//#  endif
//#elif defined(__aarch64__)
//#  define ABI "arm64-v8a"
//#elif defined(__i386__)
//#  define ABI "x86"
//#elif defined(__x86_64__)
//#  define ABI "x86-x64"
//#else
//#  define ABI "unknown"
//#endif
//		LOGI("This dynamic shared library is compiled with ABI: %s", ABI);
//		return "This native library is compiled with ABI: %s" ABI ".";
//	}
//}