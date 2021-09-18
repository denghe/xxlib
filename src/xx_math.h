#pragma once
#include <cstdint>
#include <array>
#include <cmath>
#include <cassert>

// 整数坐标系游戏常用函数/查表库。主要意图为确保跨硬件计算结果的一致性( 附带性能有一定提升 )

namespace xx {

    const double pi2 = 3.14159265358979323846264338328 * 2;

    // 设定计算坐标系 x, y 值范围 为 正负 table_xy_range
    const int table_xy_range = 2048, table_xy_range2 = table_xy_range * 2, table_xy_rangePOW2 =
            table_xy_range * table_xy_range;

    // 角度分割精度。256 对应 uint8_t，65536 对应 uint16_t, ... 对于整数坐标系来说，角度继续细分意义不大。增量体现不出来
    const int table_num_angles = 65536;
    using table_angle_element_type = uint16_t;

    // 查表 sin cos 整数值 放大系数
    const int64_t table_sincos_ratio = 10000;

    // 构造一个查表数组，下标是 +- table_xy_range 二维坐标值。内容是以 table_num_angles 为切割单位的自定义角度值( 并非 360 或 弧度 )
    inline std::array<table_angle_element_type, table_xy_range2 * table_xy_range2> table_angle;

    // 基于 table_num_angles 个角度，查表 sin cos 值 ( 通常作为移动增量 )
    inline std::array<int, table_num_angles> table_sin;
    inline std::array<int, table_num_angles> table_cos;


    // 程序启动时自动填表
    struct TableFiller {
        TableFiller() {
            // todo: 多线程计算提速
            for (int y = -table_xy_range; y < table_xy_range; ++y) {
                for (int x = -table_xy_range; x < table_xy_range; ++x) {
                    auto idx = (y + table_xy_range) * table_xy_range2 + (x + table_xy_range);
                    auto a = atan2((double) y, (double) x);
                    if (a < 0) a += pi2;
                    table_angle[idx] = (table_angle_element_type) (a / pi2 * table_num_angles);
                }
            }
            // fix same x,y
            table_angle[(0 + table_xy_range) * table_xy_range2 + (0 + table_xy_range)] = 0;

            for (int i = 0; i < table_num_angles; ++i) {
                auto s = sin((double) i / table_num_angles * pi2);
                table_sin[i] = (int) (s * table_sincos_ratio);
                auto c = cos((double) i / table_num_angles * pi2);
                table_cos[i] = (int) (c * table_sincos_ratio);
            }
        }
    };

    inline TableFiller tableFiller__;

    // 传入坐标，返回角度值( 0 ~ table_num_angles )  safeMode: 支持大于查表尺寸的 xy 值 ( 慢几倍 )
    template<bool safeMode = true>
    table_angle_element_type GetAngleXY(int x, int y) noexcept {
        if constexpr (safeMode) {
            while (x < -table_xy_range || x >= table_xy_range || y < -table_xy_range || y >= table_xy_range) {
                x /= 8;
                y /= 8;
            }
        } else {
            assert(x >= -table_xy_range && x < table_xy_range && y >= -table_xy_range && y < table_xy_range);
        }
        return table_angle[(y + table_xy_range) * table_xy_range2 + x + table_xy_range];
    }

    template<bool safeMode = true>
    table_angle_element_type GetAngleXYXY(int const &x1, int const &y1, int const &x2, int const &y2) noexcept {
        return GetAngleXY<safeMode>(x2 - x1, y2 - y1);
    }

    template<bool safeMode = true, typename Point1, typename Point2>
    table_angle_element_type GetAngle(Point1 const &from, Point2 const &to) noexcept {
        return GetAngleXY<safeMode>(to.x - from.x, to.y - from.y);
    }

    // 计算点旋转后的坐标
    template<typename XY>
    inline XY Rotate(XY const &p, table_angle_element_type const &a) noexcept {
        auto s = (int64_t) table_sin[a];
        auto c = (int64_t) table_cos[a];
        XY rtv;
        rtv.x = (int) ((p.x * c - p.y * s) / table_sincos_ratio);
        rtv.y = (int) ((p.x * s + p.y * c) / table_sincos_ratio);
        return rtv;
    }

    // distance^2 = (x1 - x1)^2 + (y1 - y2)^2
    // return (r1^2 + r2^2 > distance^2)
    template<typename XY>
    inline bool DistanceNear(int const& r1, int const& r2, XY const& p1, XY const& p2) {
        return r1 * r1 + r2 * r2 > (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
    }





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
