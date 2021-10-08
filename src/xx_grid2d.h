#pragma once
#include <cstdint>
#include <cassert>

namespace xx {

	// 2d 格子检索系统，相当于 桶 为 格子坐标 的字典( 代码经由 xx::Dict 简化而来 )
	template <typename T, typename Index_t = int16_t>
	struct Grid2d {
		struct Node {
			Index_t         next;
			Index_t         prev;
			int             bucketsIndex;           // 所在 buckets 下标
			T               value;                  // 用户数据容器
		};

		int                 rowCount;               // 行数
		int                 columnCount;            // 列数
	private:
		int                 freeList;               // 自由空间链表头( next 指向下一个未使用单元 )
		int                 freeCount;              // 自由空间链长
		int                 count;                  // 已使用空间数
		int                 itemsCapacity;          // 节点数组长度
		Node*               items;                  // 节点数组
		Index_t*            buckets;                // 桶 / 二维格子数组( 长度 = columnCount * rowCount )

	public:
		Grid2d(Grid2d const& o) = delete;
		Grid2d& operator=(Grid2d const& o) = delete;

		// 允许默认构造，但如果要调用任意成员函数，必须初始化( 这是为了方便某些初始化流程，一开始不确定尺寸 )
		explicit Grid2d()
			: rowCount(0)
			, columnCount(0)
			, freeList(-1)
			, freeCount(0)
			, count(0)
			, itemsCapacity(0)
			, items(nullptr)
			, buckets(nullptr) {}

		// 允许默认构造 + Init 事后初始化. 不可反复初始化
		void Init(int const& rowCount, int const& columnCount, int const& capacity = 0) {
			assert(!buckets);
			assert(rowCount);
			assert(columnCount);
			assert(capacity >= 0);
			this->rowCount = rowCount;
			this->columnCount = columnCount;
			this->itemsCapacity = capacity;
			items = capacity ? (Node*)malloc(capacity * sizeof(Node)) : nullptr;
			auto bucketsLen = rowCount * columnCount;
			buckets = (Index_t*)malloc(bucketsLen * sizeof(Index_t));
			memset(buckets, -1, bucketsLen * sizeof(Index_t));
		}

		// buckets 所需空间会立刻分配, 行列数不可变
		explicit Grid2d(int const& rowCount, int const& columnCount, int const& capacity = 0) : Grid2d() {
			Init(rowCount, columnCount, capacity);
		}

		~Grid2d() {
			if (!buckets) return;
			Clear();
			free(items);
			items = nullptr;
			free(buckets);
			buckets = nullptr;
		}

		void Clear() {
			assert(buckets);
			if (!count) return;
			for (int i = 0; i < count; ++i) {
				auto& o = items[i];
				if (o.bucketsIndex != -1) {
					buckets[o.bucketsIndex] = -1;
					o.bucketsIndex = -1;
					o.value.~T();
				}
			}
			freeList = -1;
			freeCount = 0;
			count = 0;
		}

		// 预分配 items 空间
		void Reserve(int const& capacity) {
			assert(buckets);
			assert(capacity > 0);
			if (capacity <= itemsCapacity) return;
			if constexpr (std::is_standard_layout_v<T> && std::is_trivial_v<T>) {
				items = (Node*)realloc((void*)items, capacity * sizeof(Node));
			} else {
				auto newNodes = (Node*)malloc(capacity * sizeof(Node));
				for (int i = 0; i < count; ++i) {
					new (&newNodes[i].value) T((T&&)items[i].value);
					items[i].value.T::~T();
				}
				free(items);
				items = newNodes;
			}
			itemsCapacity = capacity;
		}

		// 返回 items 下标
		template<typename V>
		int Add(int const& rowNumber, int const& columnNumber, V&& v) {
			assert(buckets);
			assert(rowNumber >= 0 && rowNumber < rowCount);
			assert(columnNumber >= 0 && columnNumber < columnCount);

			// alloc
			int idx;
			if (freeCount > 0) {
				idx = freeList;
				freeList = items[idx].next;
				freeCount--;
			} else {
				if (count == itemsCapacity) {
					Reserve(count ? count * 2 : 16);
				}
				idx = count;
				count++;
			}

			// calc
			auto bucketsIndex = rowNumber * columnCount + columnNumber;

			// link
			items[idx].next = buckets[bucketsIndex];
			if (buckets[bucketsIndex] >= 0) {
				items[buckets[bucketsIndex]].prev = idx;
			}
			buckets[bucketsIndex] = idx;
			items[idx].prev = -1;
			items[idx].bucketsIndex = bucketsIndex;

			// assign
			new (&items[idx].value) T(std::forward<V>(v));
			return idx;
		}

		// 根据 items 下标 移除
		void Remove(int const& idx) {
			assert(buckets);
			assert(idx >= 0 && idx < count&& items[idx].bucketsIndex != -1);

			// unlink
			if (items[idx].prev < 0) {
				buckets[items[idx].bucketsIndex] = items[idx].next;
			} else {
				items[items[idx].prev].next = items[idx].next;
			}
			if (items[idx].next >= 0) {
				items[items[idx].next].prev = items[idx].prev;
			}

			// free
			items[idx].next = freeList;
			items[idx].prev = -1;
			items[idx].bucketsIndex = -1;
			freeList = idx;
			freeCount++;

			// cleanup
			items[idx].value.~T();
		}

		// 移动 items[idx] 的所在 buckets 到 [ rowNumber * columnCount + columnNumber ]
		void Update(int const& idx, int const& rowNumber, int const& columnNumber) {
			assert(buckets);
			assert(idx >= 0 && idx < count&& items[idx].bucketsIndex != -1);
			assert(rowNumber >= 0 && rowNumber < rowCount);
			assert(columnNumber >= 0 && columnNumber < columnCount);

			// calc
			auto bucketsIndex = rowNumber * columnCount + columnNumber;

			// no change check
			if (bucketsIndex == items[idx].bucketsIndex) return;

			// unlink
			if (items[idx].prev < 0) {
				buckets[items[idx].bucketsIndex] = items[idx].next;
			} else {
				items[items[idx].prev].next = items[idx].next;
			}
			if (items[idx].next >= 0) {
				items[items[idx].next].prev = items[idx].prev;
			}


			// link
			items[idx].next = buckets[bucketsIndex];
			if (buckets[bucketsIndex] >= 0) {
				items[buckets[bucketsIndex]].prev = idx;
			}
			buckets[bucketsIndex] = idx;
			items[idx].prev = -1;
			items[idx].bucketsIndex = bucketsIndex;
		}

		// 下标访问
		Node& operator[](int const& idx) {
			assert(buckets);
			assert(idx >= 0 && idx < count);
			assert(items[idx].bucketsIndex != -1);
			return items[idx];
		}
		Node const& operator[](int const& idx) const {
			assert(buckets);
			assert(idx >= 0 && idx < count);
			assert(items[idx].bucketsIndex != -1);
			return items[idx];
		}

		// 返回 items 实际个数
		[[nodiscard]] int Count() const {
			assert(buckets);
			return count - freeCount;
		}

		[[nodiscard]] bool Empty() const {
			assert(buckets);
			return Count() == 0;
		}

		// 返回链表头 idx. -1 表示没有( for 手动遍历 )
		[[nodiscard]] int Header(int const& rowNumber, int const& columnNumber) const {
			assert(buckets);
			return buckets[rowNumber * columnCount + columnNumber];
		}

		// 遍历某个格子所有 Node 并回调 func(node.value)
		template<typename Func>
		void Find(int const& rowNumber, int const& columnNumber, Func&& func) {
			assert(buckets);
			auto idx = Header(rowNumber, columnNumber);
			if (idx == -1) return;
			do {
				assert(idx >= 0 && idx < count&& items[idx].bucketsIndex != -1);
				func(items[idx].value);
				idx = items[idx].next;
			} while (idx != -1);
		}

		// 遍历某个格子( 不可以是最边缘的) + 周围 8 格 含有哪些 Node 并回调 func(node.value)
		template<typename Func>
		void FindNeighbor(int const& rowNumber, int const& columnNumber, Func&& func) {
			assert(buckets);
			assert(rowNumber && columnNumber && rowNumber + 1 < rowCount && columnNumber + 1 < columnCount);
			Find(rowNumber, columnNumber, func);
			Find(rowNumber + 1, columnNumber, func);
			Find(rowNumber - 1, columnNumber, func);
			Find(rowNumber, columnNumber + 1, func);
			Find(rowNumber, columnNumber - 1, func);
			Find(rowNumber + 1, columnNumber + 1, func);
			Find(rowNumber + 1, columnNumber - 1, func);
			Find(rowNumber - 1, columnNumber + 1, func);
			Find(rowNumber - 1, columnNumber - 1, func);
		}

		// 有限遍历某个格子所有 Node 并回调 func(node.value). 每次回调之后会 -- limit。当 <= 0 时，停止遍历
		template<typename Func>
		void LimitFind(int& limit, int const& rowNumber, int const& columnNumber, Func&& func) {
			assert(buckets);
			assert(limit > 0);
			auto idx = Header(rowNumber, columnNumber);
			if (idx == -1) return;
			do {
				assert(idx >= 0 && idx < count&& items[idx].bucketsIndex != -1);
				func(items[idx].value);
				if (--limit <= 0) return;
				idx = items[idx].next;
			} while (idx != -1);
		}

		// 有限遍历某个格子( 不可以是最边缘的) + 周围 8 格 含有哪些 Node 并回调 func(node.value). 每次回调之后会 -- limit。当 <= 0 时，停止遍历
		template<typename Func>
		void LimitFindNeighbor(int& limit, int const& rowNumber, int const& columnNumber, Func&& func) {
			assert(buckets);
			assert(limit > 0);
			assert(rowNumber && columnNumber && rowNumber + 1 < rowCount && columnNumber + 1 < columnCount);
			LimitFind(limit, rowNumber, columnNumber, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber + 1, columnNumber, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber, columnNumber - 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber + 1, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber + 1, columnNumber - 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber - 1, func);
		}

		// 有限遍历某个 Node 所在格子 别的 Node 并回调 func(node.value). 每次回调之后会 -- limit。当 <= 0 时，停止遍历
		// 遍历时将跳过 tarIdx 本身
		template<typename Func>
		void LimitFind(int& limit, int const& tarIdx, Func&& func) {
			assert(buckets);
			assert(limit > 0);
			assert(tarIdx >= 0 && tarIdx < count);
			assert(items[tarIdx].bucketsIndex != -1);
			auto idx = buckets[items[tarIdx].bucketsIndex];
			if (idx == -1) return;
			do {
				if (idx != tarIdx) {
					assert(idx >= 0 && idx < count&& items[idx].bucketsIndex != -1);
					func(items[idx].value);
					if (--limit <= 0) return;
				}
				idx = items[idx].next;
			} while (idx != -1);
		}

		// 有限遍历某个格子( 不可以是最边缘的) + 周围 8 格 含有哪些 Node 并回调 func(node.value). 每次回调之后会 -- limit。当 <= 0 时，停止遍历
		// 遍历时将跳过 tarIdx 本身
		template<typename Func>
		void LimitFindNeighbor(int& limit, int const& tarIdx, Func&& func) {
			assert(buckets);
			assert(limit > 0);
			assert(items[tarIdx].bucketsIndex != -1);
			LimitFind(limit, tarIdx, func); if (limit <= 0) return;
			auto bidx = items[tarIdx].bucketsIndex;
			auto rowNumber = bidx / columnCount;
			auto columnNumber = bidx - rowNumber * columnCount;
			LimitFind(limit, rowNumber + 1, columnNumber, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber, columnNumber - 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber + 1, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber + 1, columnNumber - 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber + 1, func); if (limit <= 0) return;
			LimitFind(limit, rowNumber - 1, columnNumber - 1, func);
		}

		// todo: more: 支持距离计算？ 圆形扩散？
	};

}
