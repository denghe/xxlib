#include "xx_helpers.h"

// todo: compress int

// 可想象为 格子字典。key 为 格子坐标
// 代码经由 xx::Dict 简化而来，备注参考它
template <typename T, typename Index_t = int16_t>
struct Grid {
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
    Grid(Grid const& o) = delete;
    Grid& operator=(Grid const& o) = delete;

    // buckets 所需空间会立刻分配, 行列数不可变
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
        buckets = (Index_t*)malloc(bucketsLen * sizeof(Index_t));
        memset(buckets, -1, bucketsLen * sizeof(Index_t));
        items = capacity ? (Node*)malloc(capacity * sizeof(Node)) : nullptr;
    }

    ~Grid() {
        Clear();
        free(items);
        items = nullptr;
        free(buckets);
        buckets = nullptr;
    }

    void Clear() {
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
        auto bucketsIndex = rowNumber * rowCount + columnNumber;

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
        assert(idx >= 0 && idx < count && items[idx].bucketsIndex != -1);

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

    // 移动 items[idx] 的所在 buckets 到 [ rowNumber * rowCount + columnNumber ]
    void Update(int const& idx, int const& rowNumber, int const& columnNumber) {
        assert(idx >= 0 && idx < count && items[idx].bucketsIndex != -1);
        assert(rowNumber >= 0 && rowNumber < rowCount);
        assert(columnNumber >= 0 && columnNumber < columnCount);

        // unlink
        if (items[idx].prev < 0) {
            buckets[items[idx].bucketsIndex] = items[idx].next;
        } else {
            items[items[idx].prev].next = items[idx].next;
        }
        if (items[idx].next >= 0) {
            items[items[idx].next].prev = items[idx].prev;
        }

        // calc
        auto bucketsIndex = rowNumber * rowCount + columnNumber;

        // link
        items[idx].next = buckets[bucketsIndex];
        if (buckets[bucketsIndex] >= 0) {
            items[buckets[bucketsIndex]].prev = idx;
        }
        buckets[bucketsIndex] = idx;
        items[idx].prev = -1;
        items[idx].bucketsIndex = bucketsIndex;
    }

    // 查找某个格子含有哪些 idx, 填充到 out( 不清除旧数据 ), 返回填充个数
    template<typename Container>
    int Fill(int const& rowNumber, int const& columnNumber, Container& out) {
        auto idx = Header(rowNumber, columnNumber);
        if (idx == -1) return 0;
        int n = 0;
        do {
            ++n;
            out.push_back(idx);
            idx = items[idx].next;
        } while (idx != -1);
        return n;
    }

    // 返回链表头 idx. -1 表示没有. 手动遍历参考 Fill
    [[nodiscard]] int Header(int const& rowNumber, int const& columnNumber) const {
        return buckets[rowNumber * rowCount + columnNumber];
    }

    // 下标访问
    Node& operator[](int const& idx) {
        assert(items[idx].bucketsIndex != -1);
        return items[idx];
    }
    Node const& operator[](int const& idx) const {
        assert(items[idx].bucketsIndex != -1);
        return items[idx];
    }

    // 返回 items 实际个数
    [[nodiscard]] int Count() const {
        return count - freeCount;
    }

    [[nodiscard]] bool Empty() const {
        return Count() == 0;
    }
};

#include "xx_ptr.h"
#include "xx_string.h"

struct XY {
    int x, y;
};
struct Foo {
    XY xy;

    Grid<Foo*>* g;
    int idx;

    void SetXY(XY const& xy_) {
        xy = xy_;
        g->Update(idx, xy_.x, xy_.y);
    }

    ~Foo() {
        g->Remove(idx);
    }
};

int main() {
    Grid<Foo*> g(5000, 5000);
    std::vector<xx::Shared<Foo>> foos;

    auto NewFoo = [&](int x, int y) {
        auto&& f = foos.emplace_back().Emplace();
        f->xy = { x, y };
        f->g = &g;
        f->idx = g.Add(f->xy.x, f->xy.y, &*f);
    };
    NewFoo(1, 1);
    NewFoo(4, 4);
    NewFoo(2, 2);
    NewFoo(3, 3);

    xx::CoutN("foos.size() = ", foos.size());
    xx::CoutN("g.Count() = ", g.Count());

    foos[1]->SetXY({ 1, 1 });

    std::vector<int> idxs;
    auto n = g.Fill(1, 1, idxs);
    xx::CoutN("n = ", n);
    for (auto& idx : idxs) {
        xx::CoutN("idx = ", idx);
    }

    idxs.clear();
    n = g.Fill(2, 2, idxs);
    xx::CoutN("n = ", n);
    for (auto& idx : idxs) {
        xx::CoutN("idx = ", idx);
    }

    for (auto& f : foos) {
        xx::CoutN("f->idx = ", f->idx);
    }
    foos.clear();

    xx::CoutN("foos.size() = ", foos.size());
    xx::CoutN("g.Count() = ", g.Count());

    return 0;
}
