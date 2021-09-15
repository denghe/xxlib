#include "xx_helpers.h"

template <typename T>
struct Grid {
	struct Node {
		int             prev;
		int             next;
		T               value;
	};

private:
	int                 freeList;               // 自由空间链表头( next 指向下一个未使用单元 )
	int                 freeCount;              // 自由空间链长
	int                 count;                  // 已使用空间数

	int					rowCount;				// 行数
	int					columnCount;			// 列数
	int*				buckets;				// 桶 / 二维格子数组( 长度 = columnCount * rowCount )

	int					itemsCapacity;			// 节点数组长度
	Node*				items;                  // 节点数组


	void DeleteVs() noexcept {
		assert(buckets);
		for (int i = 0; i < count; ++i) {
			if (items[i].prev != -2) {
				items[i].value.~T();
				items[i].prev = -2;
			}
		}
	}
public:
	Grid(Grid const& o) = delete;
	Grid& operator=(Grid const& o) = delete;

	// 代码经由 xx::Dict 简化而来，备注参考它

	explicit Grid(int const& rowCount, int const& columnCount, int const& capacity = 0)
	: rowCount(rowCount)
	, columnCount(columnCount)
	, itemsCapacity(capacity) {
		assert(rowCount);
		assert(columnCount);
		assert(capacity >= 0);
		freeList = -1;
		freeCount = 0;
		count = 0;
		auto bucketsLen = rowCount * columnCount;
		buckets = (int*)malloc(bucketsLen * sizeof(int));
		memset(buckets, -1, bucketsLen * sizeof(int));
		items = capacity ? (Node*)malloc(capacity * sizeof(Node)) : nullptr;
	}

	~Grid() {
		DeleteVs();
		free(buckets);
		free(items);
	}

	void Clear() noexcept {
		if (!count) return;
		DeleteVs();
		auto bucketsLen = rowCount * columnCount;
		memset(buckets, -1, bucketsLen * sizeof(int));
		freeList = -1;
		freeCount = 0;
		count = 0;
	}

	void Reserve(int const& capacity) {
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
	int Add(int const& rowIndex, int const& columnIndex, V&& v) {
		// alloc
		int index;
		if (freeCount > 0) {
			index = freeList;
			freeList = items[index].next;
			freeCount--;
		} else {
			if (count == itemsCapacity) {
				Reserve(count ? count * 2 : 16);
			}
			index = count;
			count++;
		}

		// calc
		auto cellIndex = rowIndex * rowCount + columnIndex;

		// link
		items[index].next = buckets[cellIndex];
		if (buckets[cellIndex] >= 0) {
			items[buckets[cellIndex]].prev = index;
		}
		buckets[cellIndex] = index;
		items[index].prev = -1;

		// assign
		new (&items[index].value) T(std::forward<V>(v));
		return index;
	}

	// 根据 items 下标 移除
	void Remove(int const& idx) {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);

		if (items[idx].prev < 0) {
			buckets[idx] = items[idx].next;
		} else {
			items[items[idx].prev].next = items[idx].next;
		}
		if (items[idx].next >= 0) {
			items[items[idx].next].prev = items[idx].prev;
		}

		items[idx].next = freeList;
		freeList = idx;
		freeCount++;

		items[idx].value.~T();
		items[idx].prev = -2;
	}

	T& operator[](int const& idx) {
		return items[idx].value;
	}
	T const& operator[](int const& idx) const {
		return items[idx].value;
	}
};

struct XY {
	int x, y;
};
struct Foo {
	XY xy;
};

int main() {
	std::vector<Foo> foos;
	foos.emplace_back().xy = { 1, 1 };
	foos.emplace_back().xy = { 2, 2 };
	foos.emplace_back().xy = { 3, 3 };

	Grid<Foo*> g(5000, 5000);
	for (auto& f : foos) {
		g.Add(f.xy.x, f.xy.y, &f);
	}

	return 0;
}

