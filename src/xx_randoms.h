#pragma once

#include "xx_obj.h"
#include <chrono>
#include <random>

// some simple random impl

namespace xx {

    template<typename T>
    struct RandomBase {
        inline int32_t NextInt() {
            return ((T*)this)->Next() & 0x7FFFFFFFu;
        };
        inline double NextDouble() {
            return (double) ((T*)this)->Next() / (double) std::numeric_limits<uint32_t>::max();
        };
        // todo: more Next funcs?
    };

    /**************************************************************************************************************/
    /**************************************************************************************************************/

	struct Random1 : RandomBase<Random1> {
		int seed;
		static const int m = 1 << 31, a = 1103515245, c = 12345;

		inline void Reset(int const& seed = 123456789) { this->seed = seed; }

		Random1(int const& seed = 123456789) : seed(seed) {}

		Random1(Random1 const&) = default;

		Random1& operator=(Random1 const&) = default;

		Random1(Random1&& o) = default;

		Random1& operator=(Random1&& o) = default;

		inline uint32_t Next() {
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
		static const uint64_t s = 0xb5ad4eceda1ce2a9;

		inline void Reset(uint64_t const& x = 0, uint64_t const& w = 0) { this->x = x; this->w = w; }

		Random2(uint64_t const& x = 0, uint64_t const& w = 0) : x(x), w(w) {}

		Random2(Random2 const&) = default;

		Random2& operator=(Random2 const&) = default;

		Random2(Random2&& o) = default;

		Random2& operator=(Random2&& o) = default;

		inline uint32_t Next() {
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

		inline void Reset(uint64_t const& seed = 1234567891234567890) { this->seed = seed; }

		explicit Random3(uint64_t const& seed = 1234567891234567890) : seed(seed) {}

		Random3(Random3 const&) = default;

		Random3& operator=(Random3 const&) = default;

		Random3(Random3&& o) = default;

		Random3& operator=(Random3&& o) = default;

		inline uint32_t Next() {
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
		uint64_t count;
		SeedType seed;
		std::mt19937 rand;

		inline void Reset(SeedType const& seed = 1234567890, uint64_t const& count = 0) {
			this->seed = seed;
            this->count = 0;
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

		inline uint32_t Next() {
			++count;
			return (uint32_t)rand();
		}
	};

    template<typename T> struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, Random4>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.Write<needReserve>(in.count);
            d.WriteFixed<needReserve>(in.seed);
        }
        static inline int Read(Data_r& d, T& out) {
            if (int r = d.Read(out.count)) return r;
            if (int r = d.ReadFixed(out.seed)) return r;
            out.Reset(out.seed, out.count);
            return 0;
        }
    };


    /**************************************************************************************************************/
    /**************************************************************************************************************/

	template<typename T>
	struct ObjFuncs<T, std::enable_if_t<
		std::is_same_v<std::decay_t<T>, Random1>
		|| std::is_same_v<std::decay_t<T>, Random2>
		|| std::is_same_v<std::decay_t<T>, Random3>
		|| std::is_same_v<std::decay_t<T>, Random4>
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
