#pragma once

#include "xx_obj.h"
#include <chrono>
#include <random>

// some simple random impl

#define RANDOM_CALC_FUNCS \
inline int32_t NextInt() { \
    return Next() & 0x7FFFFFFFu; \
}; \
inline double NextDouble() { \
    return (double) Next() / (double) std::numeric_limits<uint32_t>::max(); \
};

// todo: more Next funcs?


namespace xx {

	struct Random1 {
		int seed;
		static const int m = 1 << 31, a = 1103515245, c = 12345;

		inline void Reset() { seed = 123456789; }

		Random1(int const& seed = 123456789) : seed(seed) {}

		Random1(Random1 const&) = default;

		Random1& operator=(Random1 const&) = default;

		Random1(Random1&& o) noexcept {
			std::swap(seed, o.seed);
		}

		Random1& operator=(Random1&& o) noexcept {
			std::swap(seed, o.seed);
			return *this;
		}

		inline uint32_t Next() {
			seed = (a * seed + c) % m;
			return (uint32_t)seed;
		}

		RANDOM_CALC_FUNCS
	};

	struct Random2 {
		uint64_t x, w;
		static const uint64_t s = 0xb5ad4eceda1ce2a9;

		inline void Reset() { x = w = 0; }

		Random2(uint64_t const& x = 0, uint64_t const& w = 0) : x(x), w(w) {}

		Random2(Random2 const&) = default;

		Random2& operator=(Random2 const&) = default;

		Random2(Random2&& o) noexcept {
			std::swap(x, o.x);
			std::swap(w, o.w);
		}

		Random2& operator=(Random2&& o) noexcept {
			std::swap(x, o.x);
			std::swap(w, o.w);
			return *this;
		}

		inline uint32_t Next() {
			x *= x;
			x += (w += s);
			return (uint32_t)(x = (x >> 32) | (x << 32));
		}

		RANDOM_CALC_FUNCS
	};

	struct Random3 {
		uint64_t seed;

		inline void Reset() { seed = 1234567891234567890; }

		Random3() = default;

		explicit Random3(uint64_t const& seed = 1234567891234567890) : seed(seed) {}

		Random3(Random3 const&) = default;

		Random3& operator=(Random3 const&) = default;

		Random3(Random3&& o) noexcept {
			std::swap(seed, o.seed);
		}

		Random3& operator=(Random3&& o) noexcept {
			std::swap(seed, o.seed);
			return *this;
		}

		inline uint32_t Next() {
			seed ^= (seed << 21u);
			seed ^= (seed >> 35u);
			seed ^= (seed << 4u);
			return (uint32_t)seed;
		}

		RANDOM_CALC_FUNCS
	};

	// deserialize maybe slow: rand.discard(count);
	struct Random4 {
		using SeedType = typename std::mt19937::result_type;
		uint64_t count;
		SeedType seed;
		std::mt19937 rand;

		inline void Reset() {
			seed = 1234567890;
			count = 0;
			rand.seed(seed);
		}

		// seed = std::random_device{}()
		Random4(SeedType const& seed = 1234567890, uint64_t const& count = 0)
			: count(count)
			, seed(seed)
			, rand(seed) {
			if (count) {
				rand.discard(count);
			}
		}

		Random4(Random4 const&) = default;

		Random4& operator=(Random4 const&) = default;

		Random4(Random4&& o) noexcept {
			std::swap(rand, o.rand);
		}

		Random4& operator=(Random4&& o) noexcept {
			std::swap(rand, o.rand);
			return *this;
		}

		// for DataReader
		inline void InitBySeedCount() {
			rand.seed(seed);
			rand.discard(count);
		}

		inline uint32_t Next() {
			++count;
			return (uint32_t)rand();
		}

		RANDOM_CALC_FUNCS
	};



	template<typename T>
	struct IsPod<T, std::enable_if_t<
		std::is_same_v<std::decay_t<T>, Random1>
		|| std::is_same_v<std::decay_t<T>, Random2>
		|| std::is_same_v<std::decay_t<T>, Random3>
		//|| std::is_same_v<std::decay_t<T>, Random4>
		>> : std::true_type {
	};



	template<typename T>
	struct ObjFuncs<T, std::enable_if_t<
		std::is_same_v<std::decay_t<T>, Random1>
		|| std::is_same_v<std::decay_t<T>, Random2>
		|| std::is_same_v<std::decay_t<T>, Random3>
		|| std::is_same_v<std::decay_t<T>, Random4>
		>> {
		static inline void Write(ObjManager& om, Data& d, T const& in) {
			if constexpr (std::is_same_v<std::decay_t<T>, Random4>) {
				d.WriteFixed(in.count);
                d.WriteFixed(in.seed);
			}
			else {
                d.WriteBuf(&in, sizeof(in));
			}
		}
        static inline void WriteFast(ObjManager& om, Data& d, T const& in) {
            if constexpr (std::is_same_v<std::decay_t<T>, Random4>) {
                d.WriteFixed<false>(in.count);
                d.WriteFixed<false>(in.seed);
            }
            else {
                d.WriteBuf<false>(&in, sizeof(in));
            }
        }
		static inline int Read(ObjManager& om, Data_r& d, T& out) {
			if constexpr (std::is_same_v<std::decay_t<T>, Random4>) {
				if (int r = d.ReadFixed(out.count)) return r;
				if (int r = d.ReadFixed(out.seed)) return r;
				out.InitBySeedCount();
				return 0;
			}
			else {
				return d.ReadBuf(&out, sizeof(T));
			}
		}
		static inline void Append(ObjManager& om, std::string& s, T const& in) {
			s.push_back('{');
			if constexpr (std::is_same_v < std::decay_t<T>, Random1 >
				|| std::is_same_v < std::decay_t<T>, Random3 >) {
				om.Append(s, "\"seed\":", in.seed);
			}
			else if constexpr (std::is_same_v < std::decay_t<T>, Random2>) {
				om.Append(s, "\"x\":", in.x, ",\"w\":", in.w);
			}
			else if constexpr (std::is_same_v < std::decay_t<T>, Random4 >) {
				om.Append(s, "\"seed\":", in.seed, ",\"count\":", in.count);
			}
			s.push_back('}');
		}
		static inline void AppendCore(ObjManager& om, std::string& s, T const& in) {
		}
		static inline void Clone(ObjManager& om, T const& in, T& out) {
			if constexpr (std::is_same_v<std::decay_t<T>, Random4>) {
				out.count = in.count;
				out.seed = in.seed;
				out.InitBySeedCount();
			}
			else {
				memcpy(&out, &in, sizeof(T));
			}
		}
		static inline int RecursiveCheck(ObjManager& om, T const& in) {
			return 0;
		}
		static inline void RecursiveReset(ObjManager& om, T& in) {
		}
		static inline void SetDefaultValue(ObjManager& om, T& in) {
			in.Reset();
		}
	};
}
