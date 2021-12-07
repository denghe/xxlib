#pragma once
#include <utility>
#include <cstring>

namespace xx {
	// 持有一段 buf 内存, 可存储 pod 类型。只提供 内存释放 和 Get自动扩容
	struct BufHolder {
		void* buf = nullptr;
		size_t cap = 0;

		BufHolder() = default;
		BufHolder(BufHolder const&) = delete;
		BufHolder& operator=(BufHolder const&) = delete;
		BufHolder(BufHolder&& o) noexcept {
			std::swap(buf, o.buf);
			std::swap(cap, o.cap);
		}
		BufHolder& operator=(BufHolder&& o) noexcept {
			std::swap(buf, o.buf);
			std::swap(cap, o.cap);
			return *this;
		}

		~BufHolder() {
			Shrink();
		}

		template<typename T, bool skipOldData = false>
		T* Get(size_t const& len) {
			auto newCap = sizeof(T) * len;
			if (newCap > cap) {
				if constexpr (skipOldData) {
					buf = realloc(buf, newCap);
				}
				else {
					free(buf);
					buf = malloc(newCap);
				}
				this->cap = newCap;
			}
			return (T*)buf;
		}

		void Shrink() {
			if (cap) {
				free(buf);
				buf = nullptr;
				cap = 0;
			}
		}
	};
}
