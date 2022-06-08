#pragma once

#include "xx_obj.h"
#include <chrono>
#include <random>

// some simple random impl

namespace xx {

    template<typename T>
    struct RandomBase {
        int32_t NextInt() {
            return ((T*)this)->Next() & 0x7FFFFFFFu;
        };
        double NextDouble() {
            return (double) ((T*)this)->Next() / (double) std::numeric_limits<uint32_t>::max();
        };
        // todo: more Next funcs?
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

	struct Random1 : RandomBase<Random1> {
		int seed;
		inline static const int m = 1 << 31, a = 1103515245, c = 12345;

		void Reset(int const& seed = 123456789) { this->seed = seed; }

		Random1(int const& seed = 123456789) : seed(seed) {}

		Random1(Random1 const&) = default;

		Random1& operator=(Random1 const&) = default;

		Random1(Random1&& o) = default;

		Random1& operator=(Random1&& o) = default;

		uint32_t Next() {
			seed = (a * seed + c) % m;
			return (uint32_t)seed;
		}
	};

    template<typename T> struct IsPod<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random1>>> : std::true_type {};

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random1>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in.seed);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixed(out.seed);
        }
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

    struct Random2 : RandomBase<Random2> {
		uint64_t x, w;
		inline static const uint64_t s = 0xb5ad4eceda1ce2a9;

		void Reset(uint64_t const& x = 0, uint64_t const& w = 0) { this->x = x; this->w = w; }

		Random2(uint64_t const& x = 0, uint64_t const& w = 0) : x(x), w(w) {}

		Random2(Random2 const&) = default;

		Random2& operator=(Random2 const&) = default;

		Random2(Random2&& o) = default;

		Random2& operator=(Random2&& o) = default;

		uint32_t Next() {
			x *= x;
			x += (w += s);
			return (uint32_t)(x = (x >> 32) | (x << 32));
		}
	};

    template<typename T> struct IsPod<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random2>>> : std::true_type {};

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random2>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in.x);
            d.WriteFixed<needReserve>(in.w);
        }
        static inline int Read(Data_r& d, T& out) {
            if (int r = d.ReadFixed(out.x)) return r;
            if (int r = d.ReadFixed(out.w)) return r;
            return 0;
        }
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

    struct Random3 : RandomBase<Random3> {
		uint64_t seed;

		void Reset(uint64_t const& seed = 1234567891234567890) { this->seed = seed; }

		explicit Random3(uint64_t const& seed = 1234567891234567890) : seed(seed) {}

		Random3(Random3 const&) = default;

		Random3& operator=(Random3 const&) = default;

		Random3(Random3&& o) = default;

		Random3& operator=(Random3&& o) = default;

		uint32_t Next() {
			seed ^= (seed << 21u);
			seed ^= (seed >> 35u);
			seed ^= (seed << 4u);
			return (uint32_t)seed;
		}
	};

    template<typename T> struct IsPod<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random3>>> : std::true_type {};

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random3>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in.seed);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixed(out.seed);
        }
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

    // deserialize maybe slow: rand.discard(count);
	struct Random4 : RandomBase<Random4> {
		using SeedType = typename std::mt19937::result_type;
		SeedType seed;
        uint64_t count;
		std::mt19937 rand;

		void Reset(SeedType const& seed = 1234567890, uint64_t const& count = 0) {
			this->seed = seed;
            this->count = count;
			rand.seed(seed);
            if (count) {
                rand.discard(count);
            }
		}

		// seed = std::random_device{}()
		Random4(SeedType const& seed = 1234567890, uint64_t const& count = 0) {
			Reset(seed, count);
		}

		Random4(Random4 const&) = default;

		Random4& operator=(Random4 const&) = default;

		Random4(Random4&& o) = default;

		Random4& operator=(Random4&& o) = default;

		uint32_t Next() {
			++count;
			return (uint32_t)rand();
		}
	};

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random4>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in.seed);
            d.Write<needReserve>(in.count);
        }
        static inline int Read(Data_r& d, T& out) {
            if (int r = d.ReadFixed(out.seed)) return r;
            if (int r = d.Read(out.count)) return r;
            out.Reset(out.seed, out.count);
            return 0;
        }
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

    // reference from https://github.com/cslarsen/mersenne-twister
    // faster than std impl, can store & restore state data directly
    // ser/de data size == 5000 bytes
    struct Random5 : RandomBase<Random5> {
        inline static const size_t SIZE = 624;
        inline static const size_t PERIOD = 397;
        inline static const size_t DIFF = SIZE - PERIOD;
        inline static const uint32_t MAGIC = 0x9908b0df;
        uint32_t MT[SIZE];
        uint32_t MT_TEMPERED[SIZE];
        size_t index = SIZE;
        uint32_t seed;
        void Reset(uint32_t const& seed = 1234567890) {
            this->seed = seed;
            MT[0] = seed;
            index = SIZE;
            for (uint_fast32_t i = 1; i < SIZE; ++i) {
                MT[i] = 0x6c078965 * (MT[i - 1] ^ MT[i - 1] >> 30) + i;
            }
        }
        Random5(uint32_t const& seed = 1234567890) {
            Reset(seed);
        }

        Random5& operator=(Random5 const&) = default;
        Random5(Random5&& o) = default;
        Random5& operator=(Random5&& o) = default;

#define Random5_M32(x) (0x80000000 & x) // 32nd MSB
#define Random5_L31(x) (0x7FFFFFFF & x) // 31 LSBs
#define Random5_UNROLL(expr) \
  y = Random5_M32(MT[i]) | Random5_L31(MT[i+1]); \
  MT[i] = MT[expr] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31) & MAGIC); \
  ++i;
        void Generate() {
            size_t i = 0;
            uint32_t y;
            while (i < DIFF) {
                Random5_UNROLL(i + PERIOD);
            }
            while (i < SIZE - 1) {
                Random5_UNROLL(i - DIFF);
            }
            {
                y = Random5_M32(MT[SIZE - 1]) | Random5_L31(MT[0]);
                MT[SIZE - 1] = MT[PERIOD - 1] ^ (y >> 1) ^ (((int32_t(y) << 31) >> 31) & MAGIC);
            }
            for (size_t i = 0; i < SIZE; ++i) {
                y = MT[i];
                y ^= y >> 11;
                y ^= y << 7 & 0x9d2c5680;
                y ^= y << 15 & 0xefc60000;
                y ^= y >> 18;
                MT_TEMPERED[i] = y;
            }
            index = 0;
        }
#undef Random5_UNROLL
#undef Random5_L31
#undef Random5_M32

        uint32_t Next() {
            if (index == SIZE) {
                Generate();
                index = 0;
            }
            return MT_TEMPERED[index++];
        }
    };

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random5>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixedArray<needReserve, uint32_t>(in.MT, T::SIZE);
            d.WriteFixedArray<needReserve, uint32_t>(in.MT_TEMPERED, T::SIZE);
            d.WriteFixed<needReserve>((uint32_t)in.index);
            d.WriteFixed<needReserve>(in.seed);
        }
        static inline int Read(Data_r& d, T& out) {
            if (int r = d.ReadFixedArray<uint32_t>(out.MT, T::SIZE)) return r;
            if (int r = d.ReadFixedArray<uint32_t>(out.MT_TEMPERED, T::SIZE)) return r;
            uint32_t idx;
            if (int r = d.ReadFixed(idx)) return r;
            out.index = idx;
            if (int r = d.ReadFixed(out.seed)) return r;
            return 0;
        }
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

    template<typename T>
    struct StringFuncs<T, std::enable_if_t<
            std::is_same_v<std::decay_t<T>, Random1>
            || std::is_same_v<std::decay_t<T>, Random2>
            || std::is_same_v<std::decay_t<T>, Random3>
            || std::is_same_v<std::decay_t<T>, Random4>
            || std::is_same_v<std::decay_t<T>, Random5>
            >> {
        static inline void Append(std::string& s, T const& in) {
            s.push_back('{');
            if constexpr (std::is_same_v < std::decay_t<T>, Random1 >
                          || std::is_same_v < std::decay_t<T>, Random3 >
                          || std::is_same_v < std::decay_t<T>, Random5 >
                ) {
                ::xx::Append(s, "\"seed\":", in.seed);
            }
            else if constexpr (std::is_same_v < std::decay_t<T>, Random2>) {
                ::xx::Append(s, "\"x\":", in.x, ",\"w\":", in.w);
            }
            else if constexpr (std::is_same_v < std::decay_t<T>, Random4 >) {
                ::xx::Append(s, "\"seed\":", in.seed, ",\"count\":", in.count);
            }
            s.push_back('}');
        }
    };

	template<typename T>
	struct ObjFuncs<T, std::enable_if_t<
		std::is_same_v<std::decay_t<T>, Random1>
		|| std::is_same_v<std::decay_t<T>, Random2>
		|| std::is_same_v<std::decay_t<T>, Random3>
		|| std::is_same_v<std::decay_t<T>, Random4>
		|| std::is_same_v<std::decay_t<T>, Random5>
		>> {
		static inline void Write(ObjManager& om, Data& d, T const& in) {
			d.Write(in);
		}
        static inline void WriteFast(ObjManager& om, Data& d, T const& in) {
            d.Write<false>(in);
        }
		static inline int Read(ObjManager& om, Data_r& d, T& out) {
		    return d.Read(out);
		}
		static inline void Append(ObjManager& om, std::string& s, T const& in) {
            ::xx::Append(s, in);
		}
		static inline void AppendCore(ObjManager& om, std::string& s, T const& in) {
		}
		static inline void Clone(ObjManager& om, T const& in, T& out) {
			if constexpr (std::is_same_v<std::decay_t<T>, Random4>) {
				out = in;
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
