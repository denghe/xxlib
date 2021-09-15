#include "xx_helpers.h"

// 可想象为 格子字典。key 为 格子坐标
// 代码经由 xx::Dict 简化而来，备注参考它
template <typename T>
struct Grid {
    struct Node {
        int             prev;                   // todo: prev, next 用 uint16_t ?
        int             next;
        int             index;                  // 指向 items 下标      // todo: 填充
        T               value;                  // 用户数据容器
    };

    int                 rowCount;               // 行数
    int                 columnCount;            // 列数

private:
    int                 freeList;               // 自由空间链表头( next 指向下一个未使用单元 )
    int                 freeCount;              // 自由空间链长
    int                 count;                  // 已使用空间数

    int*                buckets;                // 桶 / 二维格子数组( 长度 = columnCount * rowCount )

    int                 itemsCapacity;          // 节点数组长度
    Node*               items;                  // 节点数组


    void DeleteVs() noexcept {
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
        auto cellIndex = rowNumber * rowCount + columnNumber;

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

    // 移动 items[idx] 的所在 buckets 到 [ rowNumber * rowCount + columnNumber ]
    void Update(int const& idx, int const& rowNumber, int const& columnNumber) {
        // todo: remove part 1 + add part 2
        // unlink

    }

    // 查找某个格子含有哪些 idx, 填充到 out( 不清除旧数据 ), 返回填充个数
    template<typename Container>
    int Fill(int const& rowNumber, int const& columnNumber, Container& out) {
        auto idx = Header(rowNumber, columnNumber);
        if (idx == -1) return 0;
        int n = 0;
        do {
            out.push_back(idx);
            idx = items[idx].next;
            ++n;
        } while (idx != -1);
        return n;
    }

    // 返回链表头 idx. -1 表示没有. 手动遍历参考 Fill
    int Header(int const& rowNumber, int const& columnNumber) const {
        return buckets[rowNumber * rowCount + columnNumber];
    }

    // 下标访问
    Node& operator[](int const& idx) {
        assert(items[idx].prev != -2);
        return items[idx];
    }
    Node const& operator[](int const& idx) const {
        assert(items[idx].prev != -2);
        return items[idx];
    }

    // 返回 items 实际个数
    int Count() const {
        return count - freeCount;
    }

    bool Empty() const {
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
    NewFoo(1, 1);
    NewFoo(2, 2);
    NewFoo(3, 3);

    xx::CoutN("foos.size() = ", foos.size());
    xx::CoutN("g.Count() = ", g.Count());

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
