#pragma once

#include <xx_includes.h>
#include <openssl/rand.h>

namespace xx {
	void OpenSSL_RandomNextBytes(void* buf, size_t len) {
		while (!RAND_bytes((uint8_t*)buf, len));
	}

	template<typename V = int32_t, class = std::enable_if_t<std::is_arithmetic_v<V>>>
	V OpenSSL_RandomNext() {
		if constexpr (std::is_same_v<bool, std::decay_t<V>>) {
			uint8_t v;
			OpenSSL_RandomNextBytes(&v, sizeof(v));
			return v >= 128;
		} else if constexpr (std::is_integral_v<V>) {
			std::make_unsigned_t<V> v;
			OpenSSL_RandomNextBytes(&v, sizeof(V));
			if constexpr (std::is_signed_v<V>) {
				return (V)(v & std::numeric_limits<V>::max());
			} else return (V)v;
		} else if constexpr (std::is_floating_point_v<V>) {
			std::conditional_t<sizeof(V) == 4, uint32_t, uint64_t> v;
			OpenSSL_RandomNextBytes(&v, sizeof(V));
			if constexpr (sizeof(V) == 4) {
				return (float)v / 0xFFFFFFFFu;
			} else if constexpr (sizeof(V) == 8) {
				constexpr auto max53 = (1ull << 54) - 1;
				return double(v & max53) / max53;
			}
		}
		assert(false);
	}

	template<typename V>
	V OpenSSL_RandomNext(V from, V to) {
		assert(to >= from);
		if (from == to) return from;
		else {
			return from + OpenSSL_RandomNext<V>() % (to - from + 1);
		}
	}

	template<typename V>
	V OpenSSL_RandomNext(V to) {
		return OpenSSL_RandomNext(0, to);
	}
}
