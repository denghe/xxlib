#pragma once

#include <cassert>
#include <cstring>
#include <utility>

namespace xx {

	// 放入对象，得到固定下标用于快速访问
	// 只支持内存可随意移动的小对象放入. 例如智能指针啥的. 并且会用 0 来表达空的特殊值做 Clear
	template<typename T
		, typename D = std::aligned_storage_t<sizeof(T)>
		, class = std::enable_if_t<sizeof(D) >= sizeof(T) && sizeof(D) >= sizeof(int) >>
	struct PodPool {
		static_assert(sizeof(T) >= sizeof(int));
		int freeList = -1;
		int freeCount = 0;
		int count = 0;
		int cap = 0;
		D* pods = nullptr;

		~PodPool() {
			Clear();
			free(pods);
		}

		void Reserve(int newCap) {
			if (newCap < cap) return;
			pods = (D*)realloc(pods, sizeof(D) * newCap);
			cap = newCap;
		}
		template<typename U>
		int Add(U&& d) {
			int idx;
			if (freeCount > 0) {
				idx = freeList;
				freeList = *(int*)&pods[idx];
				freeCount--;
			}
			else {
				if (count == cap) {
					Reserve(cap ? cap * 2 : 64);
				}
				idx = count;
				count++;
			}
			new (&pods[idx]) T(std::forward<U>(d));
			return idx;
		}

		void Remove(int idx) {
			(*(T*)&pods[idx]).~T();
			*(int*)&pods[idx] = freeList;
			freeList = idx;
			++freeCount;
		}

		T const& operator[](int idx) const {
			return *(T*)&pods[idx];
		}

		int Count() const {
			return count - freeCount;
		}

		int Empty() const {
			return count == freeCount;
		}

		void Clear() {
			if (Empty()) return;
			char zeros[sizeof(D)] = { 0 };
			while (freeCount) {
				memset(&pods[freeList], 0, sizeof(D));
				freeList = *(int*)&pods[freeList];
				freeCount--;
			}
			for (int idx = 0; idx < count; ++idx) {
				if (memcmp(&pods[idx], &zeros, sizeof(D))) {
					(*(T*)&pods[idx]).~T();
				}
			}
			count = freeCount = 0;
			freeList = -1;
		}
	};

}
