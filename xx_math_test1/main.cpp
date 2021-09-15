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

	void Reserve(int const& capacity) noexcept {
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

	// return items 下标
	template<typename V>
	int Add(int const& cellIndex, V&& v) {
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

		items[index].next = buckets[cellIndex];
		if (buckets[cellIndex] >= 0) {
			buckets[buckets[cellIndex]].prev = index;
		}
		buckets[cellIndex] = index;

		new (&items[index].value) T(std::forward<V>(v));
		items[index].prev = -1;

		return index;
	}

	// 根据 items 下标移除一个单元
	void RemoveAt(int const& idx) noexcept {
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

	void Clear() noexcept {
		if (!count) return;
		DeleteVs();
		auto bucketsLen = rowCount * columnCount;
		memset(buckets, -1, bucketsLen * sizeof(int));
		freeList = -1;
		freeCount = 0;
		count = 0;
	}

	void DeleteVs() noexcept {
		assert(buckets);
		for (int i = 0; i < count; ++i) {
			if (items[i].prev != -2) {
				items[i].value.~T();
				items[i].prev = -2;
			}
		}
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

	return 0;
}










// 测试结论：查表比现算快几倍( safe mode ) 到 几十倍

//#include "xx_string.h"
////#ifdef _WIN32
////#include <mimalloc-new-delete.h>
////#endif
//#include "xx_math.h"
//
//// 计算直线的弧度
//template<typename Point1, typename Point2>
//float GetAngle(Point1 const& from, Point2 const& to) noexcept {
//	if (from.x == to.x && from.y == to.y) return 0.0f;
//	auto&& len_y = to.y - from.y;
//	auto&& len_x = to.x - from.x;
//	return atan2f(len_y, len_x);
//}
//
//// 计算距离
//template<typename Point1, typename Point2>
//float GetDistance(Point1 const& a, Point2 const& b) noexcept {
//	float dx = a.x - b.x;
//	float dy = a.y - b.y;
//	return sqrtf(dx * dx + dy * dy);
//}
//
//// 点围绕 0,0 为中心旋转 a 弧度   ( 角度 * (float(M_PI) / 180.0f) )
//template<typename Point>
//inline Point Rotate(Point const& pos, float const& a) noexcept {
//	auto&& sinA = sinf(a);
//	auto&& cosA = cosf(a);
//	return Point{ pos.x * cosA - pos.y * sinA, pos.x * sinA + pos.y * cosA };
//}
//
//
//
//struct XY {
//	int x, y;
//};
//
//int main() {
//	{
//		auto secs = xx::NowEpochSeconds();
//		double count = 0;
//		for (int k = 0; k < 50; ++k) {
//			for (int y = -1024; y < 1024; ++y) {
//				for (int x = -1024; x < 1024; ++x) {
//					count += atan2f((float)y, (float)x);
//				}
//			}
//		}
//		xx::CoutN(count);
//		xx::CoutN(xx::NowEpochSeconds() - secs);
//	}
//
//	auto secs = xx::NowEpochSeconds();
//	int64_t count = 0;
//	for (int k = 0; k < 50; ++k) {
//		for (int y = -1024; y < 1024; ++y) {
//			for (int x = -1024; x < 1024; ++x) {
//				//count += table_angle[(y + 1024) * 2048 + x + 1024];
//				count += xx::GetAngleXY<true>(x, y);
//			}
//		}
//	}
//	xx::CoutN(count);
//	xx::CoutN(xx::NowEpochSeconds() - secs);
//
//	//for(auto& o : table_sin) {
//	//    xx::Cout(o, " ");
//	//}
//	//xx::CoutN();
//	//for(auto& o : table_cos) {
//	//    xx::Cout(o, " ");
//	//}
//	//xx::CoutN();
//	return 0;
//}
