#pragma once
#include "xx_bits.h"
#include "xx_macro.h"
#include <cassert>
#include <cstdint>
#include <memory>

namespace xx {
	// 多 key 字典
	template<typename V, typename ...KS>
	class DictEx;

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

		template<typename V, typename ...KS>
		friend class DictEx;

	public:

		explicit Dict(int const& capacity = 16);
		~Dict();
		Dict(Dict const& o) = delete;
		Dict& operator=(Dict const& o) = delete;


		// 只支持没数据时扩容或空间用尽扩容( 如果不这样限制, 扩容时的 遍历损耗 略大 )
		void Reserve(int capacity = 0) noexcept;

		// 根据 key 返回下标. -1 表示没找到.
		int Find(TK const& key) const noexcept;

		// 根据 key 移除一条数据
		void Remove(TK const& key) noexcept;

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

		// 取数据记录数
		uint32_t Count() const noexcept;

		// 是否没有数据
		bool Empty() noexcept;

		// 试着填充数据到 outV. 如果不存在, 就返回 false
		bool TryGetValue(TK const& key, TV& outV) noexcept;

		// 同 operator[]
		template<typename K>
		TV& At(K&& key) noexcept;


		// 下标直读系列( unsafe )

		// 读下标所在 key
		TK const& KeyAt(int const& idx) const noexcept;

		// 读下标所在 value
		TV& ValueAt(int const& idx) noexcept;
		TV const& ValueAt(int const& idx) const noexcept;

		//Data const& DataAt(int idx) const noexcept;

		// 简单 check 某下标是否有效.
		bool IndexExists(int const& idx) const noexcept;


		// 修改 key 值系列
		template<typename K>
		bool Update(TK const& oldKey, K&& newKey) noexcept;

		template<typename K>
		bool UpdateAt(int const& idx, K&& newKey) noexcept;


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
		bucketsLen = (int)GetPrime(capacity, sizeof(Data));
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
		auto hashCode = std::hash<TK>{}(k);
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
		bucketsLen = (int)GetPrime(capacity, sizeof(Data));

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
	int Dict<TK, TV>::Find(TK const& k) const noexcept {
		assert(buckets);
		auto hashCode = std::hash<TK>{}(k);
		for (int i = buckets[hashCode % bucketsLen]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == hashCode && items[i].key == k) return i;
		}
		return -1;
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
	void Dict<TK, TV>::Remove(TK const &k) noexcept {
		assert(buckets);
		auto idx = Find(k);
		if (idx != -1) {
			RemoveAt(idx);
		}
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

	//	template <typename TK, typename TV>
	//	Dict<TK, TV>::Data const& Dict<TK, TV>::DataAt(int idx) const noexcept
	//	{
	//		return const_cast<Dict*>(this)->At(idx);
	//	}

	template <typename TK, typename TV>
	bool Dict<TK, TV>::IndexExists(int const& idx) const noexcept {
		return idx >= 0 && idx < count && items[idx].prev != -2;
	}


	template <typename TK, typename TV>
	template<typename K>
	bool Dict<TK, TV>::Update(TK const& oldKey, K&& newKey) noexcept {
		int idx = Find(oldKey);
		if (idx == -1) return false;
		return UpdateAt(idx, std::forward<K>(newKey));
	}

	template <typename TK, typename TV>
	template<typename K>
	bool Dict<TK, TV>::UpdateAt(int const& idx, K&& newKey) noexcept {
		assert(idx >= 0 && idx < count && items[idx].prev != -2);
		auto& node = nodes[idx];
		auto& item = items[idx];

		// 如果 hash 相等可以直接改 key 并退出函数
		auto newHashCode = std::hash<TK>{}(newKey);
		if (node.hashCode == newHashCode) {
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 位于相同 bucket, 直接改 has & key 并退出函数
		auto targetBucket = node.hashCode % (uint32_t)bucketsLen;
		auto newTargetBucket = newHashCode % (uint32_t)bucketsLen;
		if (targetBucket == newTargetBucket) {
			node.hashCode = newHashCode;
			item.key = std::forward<K>(newKey);
			return true;
		}

		// 检查 newKey 是否已存在
		for (int i = buckets[newTargetBucket]; i >= 0; i = nodes[i].next) {
			if (nodes[i].hashCode == newHashCode && items[i].key == newKey) return false;
		}

		// 简化的 RemoveAt
		if (item.prev < 0) {
			buckets[targetBucket] = node.next;
		}
		else {
			nodes[item.prev].next = node.next;
		}
		if (node.next >= 0) {
			items[node.next].prev = item.prev;
		}

		// 简化的 Add 后半段
		node.hashCode = newHashCode;
		node.next = buckets[newTargetBucket];

		if (buckets[newTargetBucket] >= 0) {
			items[buckets[newTargetBucket]].prev = idx;
		}
		buckets[newTargetBucket] = idx;

		item.key = std::forward<K>(newKey);
		item.prev = -1;

		return true;
	}











    template<typename V, typename ... KS> struct DictMK {
        using KeyTypes = std::tuple<KS...>;

        // dict, dicts 的下标应该是一致的
        Dict<std::tuple_element_t<0, KeyTypes>, V> dict;    // 主体
        std::tuple<Dict<KS, int>...> dicts;                 // value 指向主体 节点下标

        void Reserve(int const& cap) {
            dict.Reserve(cap);
            Reserve_<1>(cap);
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> Reserve_(int const& cap) {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> Reserve_(int const& cap) {
            std::get<i>(dicts).Reserve(cap);
            Reserve_<i + 1>(cap);
        }

    public:

        explicit DictMK(int const& cap = 16) {
            Reserve(cap);
        }

        template<int keyIndex = 0>
        int Find(std::tuple_element_t<keyIndex, KeyTypes> const& key) const noexcept {
            if constexpr (keyIndex == 0) {
                return dict.Find(key);
            }
            else {
                return std::get<keyIndex>(dicts).Find(key);
            }
        }
        template<int keyIndex = 0>
        bool Exists(std::tuple_element<keyIndex, KeyTypes> const& key) const noexcept {
            return Find<keyIndex>(key) != -1;
        }

        template<int keyIndex = 0>
        bool TryGetValue(std::tuple_element_t<keyIndex, KeyTypes> const& key, V& value) const noexcept {
            if constexpr (keyIndex == 0) {
                return dict.TryGetValue(key, value);
            }
            else {
                auto& d =  std::get<keyIndex>(dicts);
                auto index = d.Find(key);
                if (index == -1) return false;
                value = dict.ValueAt(index);
                return true;
            }
        }

        template<typename TV, typename...TKS>
        DictAddResult Add(TV&& value, TKS&&...keys) noexcept {
            static_assert(sizeof...(keys) == sizeof...(KS));
            return AddCore0(std::forward<TV>(value), std::forward<TKS>(keys)...);
        }

    protected:
        template<typename TV, typename TK, typename...TKS>
        XX_INLINE DictAddResult AddCore0(TV&& value, TK&& key, TKS&&...keys) noexcept {
            auto r = dict.Add(std::forward<TK>(key), std::forward<TV>(value), false);
            if (!r.success) return r;
            r = AddCore<1, TKS...>(r.index, std::forward<TKS>(keys)...);
            if (!r.success) {
                dict.RemoveAt(r.index);
            }
            return r;
        }
        template<int keyIndex, typename TK, typename...TKS>
        DictAddResult AddCore(int const& index, TK&& key, TKS&&...keys) noexcept {
            auto r = std::get<keyIndex>(dicts).Add(std::forward<TK>(key), index);
            if (r.success) {
                if constexpr( sizeof...(TKS) > 0) {
                    r = AddCore<keyIndex + 1, TKS...>(index, std::forward<TKS>(keys)...);
                }
                else return r;
            }
            if (!r.success) {
                std::get<keyIndex>(dicts).RemoveAt(index);
            }
            return r;
        }

    public:
        void RemoveAt(int const& index) noexcept {
            dict.RemoveAt(index);
            RemoveAt_<1>(index);
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> RemoveAt_(int const& index) {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> RemoveAt_(int const& index) {
            std::get<i>(dicts).RemoveAt(index);
            RemoveAt_<i + 1>(index);
        }

    public:
        template<int keyIndex = 0>
        bool Remove(std::tuple_element_t<keyIndex, KeyTypes> const& key) noexcept {
            auto index = Find<keyIndex>(key);
            if (index == -1) return false;
            RemoveAt(index);
            return true;
        }

        template<int keyIndex = 0, typename TK>
        bool UpdateAt(int const& index, TK&& newKey) noexcept {
            if constexpr(keyIndex == 0) {
                return dict.UpdateAt(index, std::forward<TK>(newKey));
            }
            else {
                return std::get<keyIndex>(dicts).UpdateAt(index, std::forward<TK>(newKey));
            }
        }

        template<int keyIndex = 0, typename TK>
        bool Update(std::tuple_element_t<keyIndex, KeyTypes> const& oldKey, TK&& newKey) noexcept {
            if constexpr(keyIndex == 0) {
                return dict.Update(oldKey, std::forward<TK>(newKey));
            }
            else {
                return std::get<keyIndex>(dicts).Update(oldKey, std::forward<TK>(newKey));
            }
        }

        template<int keyIndex = 0>
        std::tuple_element_t<keyIndex, KeyTypes> const& KeyAt(int const& index) const noexcept {
            if constexpr(keyIndex == 0) {
                return dict.KeyAt(index);
            }
            else {
                return std::get<keyIndex>(dicts).KeyAt(index);
            }
        }

        V& ValueAt(int const& index) noexcept {
            return dict.ValueAt(index);
        }
        [[nodiscard]] V const& ValueAt(int const& index) const noexcept {
            return dict.ValueAt(index);
        }

        void Clear() noexcept {
            dict.Clear();
            Clear_<1>();
        }
    protected:
        template<size_t i> std::enable_if_t<i == sizeof...(KS)> Clear_() {}
        template<size_t i> std::enable_if_t<i  < sizeof...(KS)> Clear_() {
            std::get<i>(dicts).Clear();
            Clear_<i + 1>();
        }

    public:
        [[nodiscard]] uint32_t Count() const noexcept {
            return dict.Count();
        }
    };
}
