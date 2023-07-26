#pragma once
#include "xx_helpers.h"

namespace xx {

	/************************************************************************************/
	// 质数相关

	//  < 2G 2^N
	constexpr static const int32_t primes2n[] = {
		1, 2, 3, 7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071, 262139, 524287,
		1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859, 134217689, 268435399, 536870909, 1073741789
	};

	inline int32_t GetPrime(int32_t const& capacity) noexcept {
        for(auto p : primes2n) {
            if (p >= capacity) return p;
        }
		assert(false);
		return -1;
	}

	// Dict.Add 的操作结果
	struct DictAddResult {
		bool success;
		int index;
		operator bool() const { return success; }
	};

	// 功能类似 std::unordered_map。抄自 .net 的 Dictionary
	// 主要特点：iter 是个固定数值下标。故可持有加速 Update & Remove 操作, 遍历时删除当前 iter 指向的数据安全。
	template <typename TK, typename TV>
	class Dict {
	public:
		typedef TK KeyType;
		typedef TV ValueType;

		struct Node {
			size_t			hashCode;
			int             next;
		};
		struct Data {
			TK              key;
			TV              value;
			int             prev;
		};

	private:
		int                 freeList;               // 自由空间链表头( next 指向下一个未使用单元 )
		int                 freeCount;              // 自由空间链长
		int                 count;                  // 已使用空间数
		int                 bucketsLen;             // 桶数组长( 质数 )
		int                *buckets;                // 桶数组
		Node               *nodes;                  // 节点数组
		Data               *items;                  // 数据数组( 与节点数组同步下标 )

	public:
		explicit Dict(int const& capacity = 16);
		~Dict();
		Dict(Dict const& o) = delete;
		Dict& operator=(Dict const& o) = delete;


		// 只支持没数据时扩容或空间用尽扩容( 如果不这样限制, 扩容时的 遍历损耗 略大 )
		void Reserve(int capacity = 0) noexcept;

		// 根据 key 返回下标. -1 表示没找到.
		template<typename K>
		int Find(K const& key) const noexcept;

		// 根据 key 移除一条数据
		template<typename K>
		void Remove(K const& key) noexcept;

		// 根据 下标 移除一条数据( unsafe )
		void RemoveAt(int const& idx) noexcept;

		// 规则同 std. 如果 key 不存在, 将创建 key, TV默认值 的元素出来. C++ 不方便像 C# 那样直接返回 default(TV). 无法返回临时变量的引用
		template<typename K>
		TV& operator[](K&& key) noexcept;

		// 清除所有数据
		void Clear() noexcept;

		// 放入数据. 如果放入失败, 将返回 false 以及已存在的数据的下标
		template<typename K, typename V>
		DictAddResult Add(K&& k, V&& v, bool const& override = false) noexcept;

        // 修改 key. 返回 负数 表示修改失败( -1: 旧 key 未找到. -2: 新 key 已存在 ), 0或正数 成功( index )
        template<typename K>
        int Update(TK const& oldKey, K&& newKey) noexcept;

        // 取数据记录数
		uint32_t Count() const noexcept;

		// 是否没有数据
		bool Empty() noexcept;

		// 试着填充数据到 outV. 如果不存在, 就返回 false
		bool TryGetValue(TK const& key, TV& outV) noexcept;

		// 同 operator[]
		template<typename K>
		TV& At(K&& key) noexcept;


		// 下标直肏系列( unsafe )

		// 读下标所在 key
		TK const& KeyAt(int const& idx) const noexcept;

		// 读下标所在 value
		TV& ValueAt(int const& idx) noexcept;
		TV const& ValueAt(int const& idx) const noexcept;

        // 修改 指定下标的 key. 返回 false 表示修改失败( 新 key 已存在 ). idx 不会改变
		template<typename K>
        bool UpdateAt(int const& idx, K&& newKey) noexcept;

        // 简单 check 某下标是否有效( for debug )
        bool IndexExists(int const& idx) const noexcept;

        // for (auto&& data : dict) {
		// for (auto&& iter = dict.begin(); iter != dict.end(); ++iter) {	if (iter->value...... iter.Remove()
		struct Iter {
			Dict& hs;
			int i;
			bool operator!=(Iter const& other) noexcept { return i != other.i; }
			Iter& operator++() noexcept {
				while (++i < hs.count) {
					if (hs.items[i].prev != -2) break;
				}
				return *this;
			}
			Data& operator*() { return hs.items[i]; }
			Data* operator->() { return &hs.items[i]; }
			operator int() const {
				return i;
			}
			void Remove() { hs.RemoveAt(i); }
		};
		Iter begin() noexcept {
			if (Empty()) return end();
			for (int i = 0; i < count; ++i) {
				if (items[i].prev != -2) return Iter{ *this, i };
			}
			return end();
		}
		Iter end() noexcept { return Iter{ *this, count }; }

	protected:
		// 用于 析构, Clear
		void DeleteKVs() noexcept;
	};





	template <typename TK, typename TV>
	Dict<TK, TV>::Dict(int const& capacity) {
		freeList = -1;
		freeCount = 0;
		count = 0;
		bucketsLen = GetPrime(capacity);
		buckets = (int*)malloc(bucketsLen * sizeof(int));
		memset(buckets, -1, bucketsLen * sizeof(int));  // -1 代表 "空"
		nodes = (Node*)malloc(bucketsLen * sizeof(Node));
		items = (Data*)malloc(bucketsLen * sizeof(Data));
	}

	template <typename TK, typename TV>
	Dict<TK, TV>::~Dict() {
		DeleteKVs();
		free(buckets);
		free(nodes);
		free(items);
	}

	template <typename TK, typename TV>
	template<typename K, typename V>
	DictAddResult Dict<TK, TV>::Add(K&& k, V&& v, bool const& override) noexcept {
		assert(bucketsLen);
		// hash 按桶数取模 定位到具体 链表, 扫找
		auto hashCode = Hash<std::decay_t<K>>{}(k);
		auto targetBucket = hashCode % bucketsLen;
		for (int i = buckets[targetBucket]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == hashCode && items[i].key == k) {
				if (override) {                      // 允许覆盖 value
					if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
						items[i].value.~TV();
					}
					new (&items[i].value) TV(std::forward<V>(v));
					return DictAddResult{ true, i };
				}
				return DictAddResult{ false, i };
			}
		}
		// 没找到则新增
		int index;									// 如果 自由节点链表 不空, 取一个来当容器
		if (freeCount > 0) {                        // 这些节点来自 Remove 操作. next 指向下一个
			index = freeList;
			freeList = nodes[index].next;
			freeCount--;
		}
		else {
			if (count == bucketsLen) {              // 所有空节点都用光了, Resize
				Reserve();
				targetBucket = hashCode % bucketsLen;
			}
			index = count;                         // 指向 Resize 后面的空间起点
			count++;
		}

		// 执行到此处时, freeList 要么来自 自由节点链表, 要么为 Reserve 后新增的空间起点.
		nodes[index].hashCode = hashCode;
		nodes[index].next = buckets[targetBucket];

		// 如果当前 bucket 中有 index, 则令其 prev 等于即将添加的 index
		if (buckets[targetBucket] >= 0) {
			items[buckets[targetBucket]].prev = index;
		}
		buckets[targetBucket] = index;

		// 移动复制构造写 key, value
		new (&items[index].key) TK(std::forward<K>(k));
		new (&items[index].value) TV(std::forward<V>(v));
		items[index].prev = -1;

		return DictAddResult{ true, index };
	}

	// 只支持没数据时扩容或空间用尽扩容( 如果不这样限制, 扩容时的 遍历损耗 略大 )
	template <typename TK, typename TV>
	void Dict<TK, TV>::Reserve(int capacity) noexcept {
		assert(buckets);
		assert(count == 0 || count == bucketsLen);          // 确保扩容函数使用情型

		// 得到空间利用率最高的扩容长度并直接修改 bucketsLen( count 为当前数据长 )
		if (capacity == 0) {
			capacity = count * 2;                           // 2倍扩容
		}
		if (capacity <= bucketsLen) return;
		bucketsLen = GetPrime(capacity);

		// 桶扩容并全部初始化( 后面会重新映射一次 )
		free(buckets);
		buckets = (int*)malloc(bucketsLen * sizeof(int));
		memset(buckets, -1, bucketsLen * sizeof(int));

		// 节点数组扩容( 保留老数据 )
		nodes = (Node*)realloc(nodes, bucketsLen * sizeof(Node));

		// item 数组扩容
		if constexpr (std::is_trivial_v<TK> && std::is_trivial_v<TV>) {
			items = (Data*)realloc((void*)items, bucketsLen * sizeof(Data));
		}
		else {
			auto newItems = (Data*)malloc(bucketsLen * sizeof(Data));
			for (int i = 0; i < count; ++i) {
				new (&newItems[i].key) TK((TK&&)items[i].key);
				if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
					items[i].key.TK::~TK();
				}
				new (&newItems[i].value) TV((TV&&)items[i].value);
				if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
					items[i].value.TV::~TV();
				}
			}
			free(items);
			items = newItems;
		}

		// 遍历所有节点, 重构桶及链表( 扩容情况下没有节点空洞 )
		for (int i = 0; i < count; i++) {
			auto index = nodes[i].hashCode % bucketsLen;
			if (buckets[index] >= 0) {
				items[buckets[index]].prev = i;
			}
			items[i].prev = -1;
			nodes[i].next = buckets[index];
			buckets[index] = i;
		}
	}

	template <typename TK, typename TV>
	template<typename K>
	int Dict<TK, TV>::Find(K const& k) const noexcept {
		assert(buckets);
		auto hashCode = Hash<std::decay_t<K>>{}(k);
		for (int i = buckets[hashCode % bucketsLen]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == hashCode && items[i].key == k) return i;
		}
		return -1;
	}

	template <typename TK, typename TV>
	template<typename K>
	void Dict<TK, TV>::Remove(K const& k) noexcept {
		assert(buckets);
		auto idx = Find(k);
		if (idx != -1) {
			RemoveAt(idx);
		}
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::RemoveAt(int const& idx) noexcept {
		assert(buckets);
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		if (items[idx].prev < 0) {
			buckets[nodes[idx].hashCode % bucketsLen] = nodes[idx].next;
		}
		else {
			nodes[items[idx].prev].next = nodes[idx].next;
		}
		if (nodes[idx].next >= 0) {      // 如果存在当前节点的下一个节点, 令其 prev 指向 上一个节点
			items[nodes[idx].next].prev = items[idx].prev;
		}

		nodes[idx].next = freeList;     // 当前节点已被移出链表, 令其 next 指向  自由节点链表头( next 有两种功用 )
		freeList = idx;
		freeCount++;

		if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
			items[idx].key.~TK();
		}
		if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
			items[idx].value.~TV();
		}
		items[idx].prev = -2;           // foreach 时的无效标志
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::Clear() noexcept {
		assert(buckets);
		if (!count) return;
		DeleteKVs();
		memset(buckets, -1, bucketsLen * sizeof(int));
		freeList = -1;
		freeCount = 0;
		count = 0;
	}

	template <typename TK, typename TV>
	template<typename K>
	TV& Dict<TK, TV>::operator[](K &&k) noexcept {
		assert(buckets);
		auto&& r = Add(std::forward<K>(k), TV(), false);
		return items[r.index].value;
		// return items[Add(std::forward<K>(k), TV(), false).index].value;		// 这样写会导致 gcc 先记录下 items 的指针，Add 如果 renew 了 items 就 crash
	}

	template <typename TK, typename TV>
	void Dict<TK, TV>::DeleteKVs() noexcept {
		assert(buckets);
		for (int i = 0; i < count; ++i) {
			if (items[i].prev != -2) {
				if constexpr (!(std::is_standard_layout_v<TK> && std::is_trivial_v<TK>)) {
					items[i].key.~TK();
				}
				if constexpr (!(std::is_standard_layout_v<TV> && std::is_trivial_v<TV>)) {
					items[i].value.~TV();
				}
				items[i].prev = -2;
			}
		}
	}

	template <typename TK, typename TV>
	uint32_t Dict<TK, TV>::Count() const noexcept {
		assert(buckets);
		return uint32_t(count - freeCount);
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::Empty() noexcept {
		assert(buckets);
		return Count() == 0;
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::TryGetValue(TK const& key, TV& outV) noexcept {
		int idx = Find(key);
		if (idx >= 0) {
			outV = items[idx].value;
			return true;
		}
		return false;
	}

	template <typename TK, typename TV>
	template<typename K>
	TV& Dict<TK, TV>::At(K&& key) noexcept {
		return operator[](std::forward<K>(key));
	}

	template <typename TK, typename TV>
	TV& Dict<TK, TV>::ValueAt(int const& idx) noexcept {
		assert(buckets);
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		return items[idx].value;
	}

	template <typename TK, typename TV>
	TK const& Dict<TK, TV>::KeyAt(int const& idx) const noexcept {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		return items[idx].key;
	}

	template <typename TK, typename TV>
	TV const& Dict<TK, TV>::ValueAt(int const& idx) const noexcept {
		return const_cast<Dict*>(this)->ValueAt(idx);
	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::IndexExists(int const& idx) const noexcept {
		return idx >= 0 && idx < count && items[idx].prev != -2;
	}

	template <typename TK, typename TV>
	template<typename K>
	int Dict<TK, TV>::Update(TK const& oldKey, K&& newKey) noexcept {
		int idx = Find(oldKey);
		if (idx == -1) return -1;
		return UpdateAt(idx, std::forward<K>(newKey)) ? idx : -2;
	}

	template <typename TK, typename TV>
	template<typename K>
    bool Dict<TK, TV>::UpdateAt(int const& idx, K&& newKey) noexcept {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);

		// 算 newKey hash, 定位到桶
		auto newHashCode = Hash<TK>{}(newKey);
        auto newBucket = newHashCode % (uint32_t)bucketsLen;

        // 检查 newKey 是否已存在
        for (int i = buckets[newBucket]; i >= 0; i = nodes[i].next) {
            if (nodes[i].hashCode == newHashCode && items[i].key == newKey) return false;
        }

        auto& node = nodes[idx];
        auto& item = items[idx];

        // 如果 hash 相等可以直接改 key 并退出
		if (node.hashCode == newHashCode) {
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 定位到旧桶
        auto oldBucket = node.hashCode % (uint32_t)bucketsLen;

		// 位于相同 bucket, 直接改 hash & key 并退出
		if (oldBucket == newBucket) {
			node.hashCode = newHashCode;
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 简化的 RemoveAt
		if (item.prev < 0) {
			buckets[oldBucket] = node.next;
		}
		else {
			nodes[item.prev].next = node.next;
		}
		if (node.next >= 0) {
			items[node.next].prev = item.prev;
		}

		// 简化的 Add 后半段
		node.hashCode = newHashCode;
		node.next = buckets[newBucket];

		if (buckets[newBucket] >= 0) {
			items[buckets[newBucket]].prev = idx;
		}
		buckets[newBucket] = idx;

		item.key = std::forward<K>(newKey);
		item.prev = -1;

		return true;
	}
}
