#include "xx_linkpool.h"
#include <vector>
#include <tuple>
#include <array>

namespace xx {
	// 模拟一下 ECS 的内存分布特性（ 按业务需求分组的 数据块 连续高密存放 )
	// 组合优先于继承, 拆分后最好只有数据, 没有函数

	// todo: 增加 排序，堆排，折半find 等功能？

	/*********************************************************************/
	// 基础库代码

	// 是否启用 combo 对象的双向链表( 可直接遍历 ). 通常会有自己的容器，用不到这个特性

#define XX_COMBO_POOL_ENABLE_FOREACH_COMBOS 0


// 每个 非 pod 切片, 需包含这个宏, 确保 move 操作的高效（ 省打字 )

#define XX_MOVEONLY_STRUCT_BASE_CODE(T)	\
T() = default;							\
T(T const&) = delete;					\
T& operator=(T const&) = delete;		\
T(T&&) = default;						\
T& operator=(T&&) = default;


	// 组合后的类的基础数据 int 包装, 便于 tuple 类型查找定位

	template<typename T>
	struct Index {
		int i;
		operator int& () { return i; }
		operator int const& () const { return i; }
	};


	// 组合体( tuple index 容器使用 ). 可 using / typedef 来定义类型 以方便使用. 也可以派生

	template<typename...TS>
	struct Combo {
		XX_MOVEONLY_STRUCT_BASE_CODE(Combo);
#if XX_COMBO_POOL_ENABLE_FOREACH_COMBOS
		int _next_, _prev_;							// _next_ 布置在这里，让 LinkPool 入池后覆盖使用
#endif
		uint32_t _refCount_, _version_;				// Shared 只关心 _refCount_. Weak 可能脏读 _version_ ( 确保不被 LinkPool 覆盖 )
		std::tuple<Index<TS>...> _sliceIndexs_;
	};


	// 判断 tuple 里是否存在某种数据类型

	template <typename T, typename Tuple>
	struct HasType;

	template <typename T>
	struct HasType<T, std::tuple<>> : std::false_type {};

	template <typename T, typename U, typename... Ts>
	struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {};

	template <typename T, typename... Ts>
	struct HasType<T, std::tuple<T, Ts...>> : std::true_type {};

	template <typename T, typename Tuple>
	using TupleContainsType = typename HasType<T, Tuple>::type;

	template <typename T, typename Tuple>
	constexpr bool TupleContainsType_v = TupleContainsType<T, Tuple>::value;


	// 计算某类型在 tuple 里是第几个

	template <class T, class Tuple>
	struct TupleTypeIndex;

	template <class T, class...TS>
	struct TupleTypeIndex<T, std::tuple<T, TS...>> {
		static const size_t value = 0;
	};

	template <class T, class U, class... TS>
	struct TupleTypeIndex<T, std::tuple<U, TS...>> {
		static const size_t value = 1 + TupleTypeIndex<T, std::tuple<TS...>>::value;
	};

	template <typename T, typename Tuple>
	constexpr size_t TupleTypeIndex_v = TupleTypeIndex<T, Tuple>::value;


	// 切片容器 tuple 套 vector

	template<typename...TS>
	struct SlicesContainer {
		using Types = std::tuple<TS...>;
		std::tuple<std::vector<TS>...> vectors;
		std::array<std::vector<std::tuple<int, int, int>>, sizeof...(TS)> typeId_Idx_IdxAddrss;	// typeId, idx, int address offset

		template<typename T> std::vector<T> const& Get() const { return std::get<std::vector<T>>(vectors); }
		template<typename T> std::vector<T>& Get() { return std::get<std::vector<T>>(vectors); }
		template<typename T> std::vector<std::tuple<int, int, int>> const& GetEx() const { return typeId_Idx_IdxAddrss[TupleTypeIndex_v<T, Types>]; }
		template<typename T> std::vector<std::tuple<int, int, int>>& GetEx() { return typeId_Idx_IdxAddrss[TupleTypeIndex_v<T, Types>]; }

		template<size_t i = 0>	std::enable_if_t<i == sizeof...(TS)> Reserve_(int const& cap) {}
		template<size_t i = 0>	std::enable_if_t < i < sizeof...(TS)> Reserve_(int const& cap) {
			std::get<i>(vectors).reserve(cap);
			Reserve_<i + 1>(cap);
		}
		void Reserve(int const& cap) {
			Reserve_(cap);
			for (auto& v : typeId_Idx_IdxAddrss) {
				v.reserve(cap);
			}
		}
	};

	// Combo 容器 tuple 套 LinkPool ( 确保分配出来的 idx 不变 )

	template<typename...TS>
	struct CombosContainer {
		using Types = std::tuple<TS...>;
		std::tuple<LinkPool<TS>...> linkpools;
		template<typename T> LinkPool<T> const& Get() const { return std::get<LinkPool<T>>(linkpools); }
		template<typename T> LinkPool<T>& Get() { return std::get<LinkPool<T>>(linkpools); }

		typedef int*(*GetIntsFunc)(std::tuple<LinkPool<TS>...>&, int const&);
		std::array<GetIntsFunc, sizeof...(TS)> getDataFuncs;

		CombosContainer() {
			FillGetIntsFuncs();
		}

		template<typename T>
		static int* GetInts(std::tuple<LinkPool<TS>...>& lps, int const& idx) {
			return (int*)&std::get<LinkPool<T>>(lps)[idx]._sliceIndexs_;
		}

		template<size_t i = 0>	std::enable_if_t<i == sizeof...(TS)> FillGetIntsFuncs() {}
		template<size_t i = 0>	std::enable_if_t< i < sizeof...(TS)> FillGetIntsFuncs() {
			using T = std::decay_t<decltype(std::get<i>(std::declval<Types>()))>;
			getDataFuncs[i] = GetInts<T>;
			FillGetIntsFuncs<i + 1>();
		}

		int* GetInts(int const& typeId, int const& idx) { return getDataFuncs[typeId](linkpools, idx); }

		template<size_t i = 0>	std::enable_if_t<i == sizeof...(TS)> Reserve_(int const& cap) {}
		template<size_t i = 0>	std::enable_if_t < i < sizeof...(TS)> Reserve_(int const& cap) {
			std::get<i>(linkpools).Reserve(cap);
			Reserve_<i + 1>(cap);
		}
		void Reserve(int const& cap) {
			Reserve_(cap);
		}
	};

	template<typename SLICES_CONTAINER, typename COMBOS_CONTAINER>
	struct ComboPool {
		// 切片容器
		SLICES_CONTAINER slices;
		// 组合容器
		COMBOS_CONTAINER combos;
		// 用于版本号自增填充
		uint32_t versionGen = 0;
#if XX_COMBO_POOL_ENABLE_FOREACH_COMBOS
		// 针对每种 combo 的 链表头, 方便分类遍历
		std::array<int, std::tuple_size_v<typename COMBOS_CONTAINER::Types>> headers;
		template<typename T>
		int& Header() {
			return headers[TupleTypeIndex_v<T, typename COMBOS_CONTAINER::Types>];
		}
		ComboPool() {
			headers.fill(-1);
		}
#endif

		void Reserve(int const& cap) {
			slices.Reserve(cap);
			combos.Reserve(cap);
		}

		template<typename T>
		void CreateData(int const& typeId, int const& owner, int const& offset, int& idx) {
			auto& s = slices.Get<T>();
			auto& n = slices.GetEx<T>();
			idx = (int)s.size();
			s.emplace_back();
			n.emplace_back(typeId, owner, offset);
			assert(s.size() == n.size());
		}
		template<typename T>
		void ReleaseData(int const& idx) {
			auto& s = slices.Get<T>();
			auto& n = slices.GetEx<T>();
			assert(idx >= 0 && idx < s.size());
			{
				auto& p = n.back();
				auto ints = combos.GetInts(std::get<0>(p), std::get<1>(p));
				ints[std::get<2>(p)] = idx;
			}
			n[idx] = std::move(n.back());
			n.pop_back();
			s[idx] = std::move(s.back());
			s.pop_back();
			assert(s.size() == n.size());
		}

		template<size_t i = 0, typename Tp, typename T>	std::enable_if_t<i == std::tuple_size_v<Tp>> TryCreate(int const& typeId, int const& owner, T& r) {}
		template<size_t i = 0, typename Tp, typename T>	std::enable_if_t < i < std::tuple_size_v<Tp>> TryCreate(int const& typeId, int const& owner, T& r) {
			using O = std::decay_t<decltype(std::get<i>(std::declval<Tp>()))>;
			if constexpr (TupleContainsType_v<Index<O>, T>) {
				CreateData<O>(typeId, owner, TupleTypeIndex_v<Index<O>, T>, std::get<Index<O>>(r));
			}
			TryCreate<i + 1, Tp, T>(typeId, owner, r);
		}

		template<size_t i = 0, typename Tp, typename T>	std::enable_if_t<i == std::tuple_size_v<Tp>> TryRelease(T const& r) {}
		template<size_t i = 0, typename Tp, typename T>	std::enable_if_t < i < std::tuple_size_v<Tp>> TryRelease(T const& r) {
			using O = std::decay_t<decltype(std::get<i>(std::declval<Tp>()))>;
			if constexpr (TupleContainsType_v<Index<O>, T>) {
				ReleaseData<O>(std::get<Index<O>>(r));
			}
			TryRelease<i + 1, Tp, T>(r);
		}




		template<typename T>
		struct Shared {
			using ElementType = T;
			ComboPool* cp;
			int idx;

			~Shared() {
				Reset();
			}
			Shared() : cp(nullptr), idx(-1) {
			}
			Shared(ComboPool* const& cp, int const& idx) : cp(cp), idx(idx) {
			}
			Shared(Shared&& o) noexcept {
				cp = o.cp;
				idx = o.idx;
				o.cp = nullptr;
			}
			Shared(Shared const& o) : cp(o.cp), idx(o.idx) {
				if (cp) {
					++cp->RefCombo<T>(idx)._refCount_;
				}
			}

			Shared& operator=(Shared const& o) {
				Reset(o.cp, o.idx);
				return *this;
			}
			Shared& operator=(Shared&& o) {
				std::swap(cp, o.cp);
				std::swap(idx, o.idx);
				return *this;
			}

			bool operator==(Shared const& o) const noexcept { return cp == o.cp && idx == o.idx; }
			bool operator!=(Shared const& o) const noexcept { return cp != o.cp || idx != o.idx; }

			template<typename Slice>
			Slice& Visit() {
				assert(cp);
				return cp->RefSlice<Slice>(cp->RefCombo<T>(idx));
			}

			T* const& operator->() const noexcept {
				assert(cp);
				return &cp->RefCombo<T>(idx);
			}

			explicit operator bool() const noexcept { return cp != nullptr; }
			bool Empty() const noexcept { return cp == nullptr; }
			uint32_t _refCount_() const noexcept { if (!cp) return 0; return cp->RefCombo(idx)._refCount_; }

			void Reset() {
				if (!cp) return;
				auto& o = cp->RefCombo<T>(idx);
				assert(o._refCount_);
				if (--o._refCount_ == 0) {
					cp->ReleaseCombo<T>(idx);
				}
				cp = nullptr;
			}

			void Reset(ComboPool* const& cp, int const& idx) {
				if (this->cp == cp && this->idx = idx) return;
				Reset();
				if (cp) {
					this->cp = cp;
					this->idx = idx;
					auto& o = cp->RefCombo<T>(idx);
					this->_version_ = o._version_;
					++o._refCount_;
				}
			}
		};

		template<typename T>
		struct Weak {
			using ElementType = T;
			ComboPool* cp;
			int idx;
			uint32_t _version_;

			Weak() : cp(nullptr), idx(-1), _version_(0) {
			}
			Weak(ComboPool* const& cp, int const& idx, uint32_t const& _version_) : cp(cp), idx(idx), _version_(_version_) {
			}
			Weak(Weak&& o) noexcept {
				cp = o.cp;
				idx = o.idx;
				_version_ = o._version_;
				o.cp = nullptr;
			}
			Weak(Weak const& o) : cp(o.cp), idx(o.idx), _version_(o._version_) {
			}

			Weak& operator=(Weak const& o) {
				cp = o.cp;
				idx = o.idx;
				_version_ = o._version_;
				return *this;
			}
			Weak& operator=(Weak&& o) {
				cp = o.cp;
				idx = o.idx;
				_version_ = o._version_;
				o.cp = nullptr;
				return *this;
			}

			bool operator==(Weak const& o) const noexcept { return cp == o.cp && idx == o.idx && _version_ == o._version_; }
			bool operator!=(Weak const& o) const noexcept { return cp != o.cp || idx != o.idx || _version_ != o._version_; }


			Weak& operator=(Shared<T> const& o) {
				Reset(o);
				return *this;
			}

			Weak(Shared<T> const& o) {
				Reset(o);
			}

			explicit operator bool() const noexcept {
				return cp && cp->RefCombo<T>(idx)._version_ == _version_;
			}

			void Reset() {
				cp = nullptr;
			}

			void Reset(Shared<T> const& s) {
				if (cp = s.cp) {
					idx = s.idx;
					_version_ = cp->RefCombo<T>(idx)._version_;
				}
			}

			Shared<T> Lock() const {
				if (!cp) return {};
				auto& o = cp->RefCombo<T>(idx);
				if (o._version_ != _version_) return {};
				++o._refCount_;
				return { cp, idx };
			}
		};




		template<typename T>
		Shared<T> CreateComboShared() {
			int idx = CreateCombo<T>();
			++combos.Get<T>()[idx]._refCount_;
			return Shared<T>(this, idx);
		}

		template<typename T>
		int CreateCombo() {
			auto& s = combos.Get<T>();
			int idx;
			s.New(idx);
			auto& r = s[idx];
			TryCreate<0, typename SLICES_CONTAINER::Types>(TupleTypeIndex_v<T, typename COMBOS_CONTAINER::Types>, idx, r._sliceIndexs_);
			r._version_ = ++versionGen;
			r._refCount_ = 0;
#if XX_COMBO_POOL_ENABLE_FOREACH_COMBOS
			auto& h = Header<T>();
			r._prev_ = -1;
			if (h != -1) {
				s[h]._prev_ = idx;
			}
			r._next_ = h;
			h = idx;
#endif
			return idx;
		}

		template<typename T>
		void ReleaseCombo(int const& idx) {
			auto& s = combos.Get<T>();
			auto& r = s[idx];
			assert(r._refCount_ == 0);
			r._version_ = 0;
			TryRelease<0, typename SLICES_CONTAINER::Types>(r._sliceIndexs_);
#if XX_COMBO_POOL_ENABLE_FOREACH_COMBOS
			auto& h = Header<T>();
			if (h == idx) {
				h = r._next_;
			}
			else/* if (r._prev_ != -1)*/ {
				s[r._prev_]._next_ = r._next_;
			}
			if (r._next_ != -1) {
				s[r._next_]._prev_ = r._prev_;
		}
#endif
			s.Delete(idx);
	}


		template<typename Combo>
		Combo& RefCombo(int const& idx) {
			return combos.Get<Combo>()[idx];
		}
		template<typename Combo>
		Combo const& RefCombo(int const& idx) const {
			return combos.Get<Combo>()[idx];
		}

		template<typename Slice, typename Combo>
		Slice& RefSlice(Combo& o) {
			auto&& idx = std::get<Index<Slice>>(o._sliceIndexs_);
			return slices.Get<Slice>()[idx];
		}
		template<typename Slice, typename Combo>
		Slice const& RefSlice(Combo const& o) const {
			auto&& idx = std::get<Index<Slice>>(o._sliceIndexs_);
			return slices.Get<Slice>()[idx];
		}

#if XX_COMBO_POOL_ENABLE_FOREACH_COMBOS
		template<typename Combo, typename F>
		void ForeachCombos(F&& f) {
			int h = Header<Combo>();
			while (h != -1) {
				auto& o = RefCombo<Combo>(h);
				h = o._next_;
				f(o);
		}
}
#endif

		template<typename Slice, typename F>
		void ForeachSlices(F&& f) {
			auto& s = slices.Get<Slice>();
			auto& n = slices.GetEx<Slice>();
			auto siz = s.size();
			for (size_t i = 0; i < siz; ++i) {
				f(s[i], n[i]);
			}
		}
	};
}
