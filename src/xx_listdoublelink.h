#pragma once
#include "xx_typetraits.h"

namespace xx {

	// double link + version's xx::ListLink but performance slowly 1/2
	// Example at the bottom of the code

    template<typename IndexType = ptrdiff_t, typename VersionType = size_t>
    struct ListDoubleLinkIndexAndVersion {
        IndexType index{ -1 };
        VersionType version{ 0 };
    };

    template<typename T, typename IndexType = ptrdiff_t, typename VersionType = size_t>
	struct ListDoubleLink {

		struct Node {
			IndexType next, prev;
			VersionType version;
			T value;
		};

		using IndexAndVersion =  ListDoubleLinkIndexAndVersion<IndexType, VersionType>;

		Node* buf{};
		IndexType cap{}, len{};
		IndexType head{ -1 }, tail{ -1 }, freeHead{ -1 }, freeCount{};
		VersionType version{};

		ListDoubleLink() = default;
		ListDoubleLink(ListDoubleLink const&) = delete;
		ListDoubleLink& operator=(ListDoubleLink const&) = delete;
		ListDoubleLink(ListDoubleLink&& o) noexcept {
			buf = o.buf;
			cap = o.cap;
			len = o.len;
			head = o.head;
			tail = o.tail;
			freeHead = o.freeHead;
			freeCount = o.freeCount;
			o.buf = {};
			o.cap = {};
			o.len = {};
			o.head = -1;
			o.tail = -1;
			o.freeHead = -1;
			o.freeCount = {};
		}
		ListDoubleLink& operator=(ListDoubleLink&& o) noexcept {
			std::swap(buf, o.buf);
			std::swap(cap, o.cap);
			std::swap(len, o.len);
			std::swap(head, o.head);
			std::swap(tail, o.tail);
			std::swap(freeHead, o.freeHead);
			std::swap(freeCount, o.freeCount);
			return *this;
		}
		~ListDoubleLink() {
			Clear<true>();
		}

		void Reserve(IndexType const& newCap) noexcept {
			assert(newCap > 0);
			if (newCap <= cap) return;
			cap = newCap;
			if constexpr (IsPod_v<T>) {
				buf = (Node*)realloc(buf, sizeof(Node) * newCap);
			} else {
				auto newBuf = (Node*)::malloc(newCap * sizeof(Node));
				for (IndexType i = 0; i < len; ++i) {
					newBuf[i].prev = buf[i].prev;
					newBuf[i].next = buf[i].next;
					newBuf[i].version = buf[i].version;
					new (&newBuf[i].value) T((T&&)buf[i].value);
					buf[i].value.~T();
				}
				::free(buf);
				buf = newBuf;
			}
		}

		IndexAndVersion Head() const {
			assert(head >= 0);
			return { head, buf[head].version };
		}

		IndexAndVersion Tail() const {
			assert(tail >= 0);
			return { tail, buf[tail].version };
		}


		//auto&& o = new (&ll.AddCore()) T( ... );
		template<bool add_to_tail = true>
		[[nodiscard]] T& AddCore() {

			IndexType idx;
			if (freeCount > 0) {
				idx = freeHead;
				freeHead = buf[idx].next;
				freeCount--;
			} else {
				if (len == cap) {
					Reserve(cap ? cap * 2 : 8);
				}
				idx = len;
				len++;
			}

			if constexpr (add_to_tail) {
				buf[idx].prev = tail;
				buf[idx].next = -1;

				if (tail >= 0) {
					buf[tail].next = idx;
					tail = idx;
				} else {
					assert(head == -1);
					head = tail = idx;
				}
			} else {
				buf[idx].prev = -1;
				buf[idx].next = head;

				if (head >= 0) {
					buf[head].prev = idx;
					head = idx;
				} else {
					assert(head == -1);
					head = tail = idx;
				}
			}

			buf[idx].version = ++version;
			return buf[idx].value;
		}

		// auto& o = Add()( ... ll.Tail() ... );
		template<bool add_to_tail = true, typename U = T>
		[[nodiscard]] TCtor<U> Add() {
			return { (U*)&AddCore<add_to_tail>() };
		}

		template<bool add_to_tail = true, typename U = T, typename...Args>
		U& Emplace(Args&&...args) {
			return *new (&AddCore<add_to_tail>()) U(std::forward<Args>(args)...);
		}

		bool Exists(IndexType const& idx) const {
			assert(idx >= 0);
			if (idx >= len) return false;
			return buf[idx].version >= 0;
		}
		bool Exists(IndexAndVersion const& iv) const {
			if (iv.index < 0 || iv.index >= len) return false;
			return buf[iv.index].version == iv.version;
		}

		void Remove(IndexType const& idx) {
			assert(Exists(idx));

			auto& node = buf[idx];
			auto prev = node.prev;
			auto next = node.next;
			if (idx == head) {
				head = next;
			}
			if (idx == tail) {
				tail = prev;
			}
			if (prev >= 0) {
				buf[prev].next = next;
			}
			if (next >= 0) {
				buf[next].prev = prev;
			}

			node.value.~T();
			node.next = freeHead;
			node.prev = -1;
			node.version = 0;
			freeHead = idx;
			++freeCount;
		}
		bool Remove(IndexAndVersion const& iv) {
			if (!Exists(iv)) return false;
			Remove(iv.index);
			return true;
		}

		IndexAndVersion ToIndexAndVersion(IndexType const& idx) const {
			return { idx, buf[idx].version };
		}

		IndexType Next(IndexAndVersion const& iv) const {
			assert(Exists(iv));
			return buf[iv.index].next;
		}

		IndexType Next(IndexType const& idx) const {
			assert(Exists(idx));
			return buf[idx].next;
		}

		IndexType Prev(IndexType const& idx) const {
			assert(Exists(idx));
			return buf[idx].prev;
		}

		T const& At(IndexType const& idx) const {
			assert(Exists(idx));
			return buf[idx].value;
		}

		T& At(IndexType const& idx) {
			assert(Exists(idx));
			return buf[idx].value;
		}

		T const& At(IndexAndVersion const& iv) const {
			assert(Exists(iv));
			return buf[iv.index].value;
		}

		T& At(IndexAndVersion const& iv) {
			assert(Exists(iv));
			return buf[iv.index].value;
		}



		template<bool freeBuf = false, bool zeroVersion = false>
		void Clear() {

			if (!cap) return;
			if constexpr (!IsPod_v<T>) {
				while (head >= 0) {
					buf[head].value.~T();
					head = buf[head].next;
				}
			}
			if constexpr (freeBuf) {
				::free(buf);
				buf = {};
				cap = 0;
			}

			head = tail = freeHead = -1;
			freeCount = 0;
			len = 0;

			if constexpr (zeroVersion) {
				version = 0;
			}
		}

		T const& operator[](IndexType const& idx) const noexcept {
			assert(Exists(idx));
			return buf[idx].value;
		}

		T& operator[](IndexType const& idx) noexcept {
			assert(Exists(idx));
			return buf[idx].value;
		}

		T const& operator[](IndexAndVersion const& iv) const noexcept {
			assert(Exists(iv));
			return buf[iv.index].value;
		}

		T& operator[](IndexAndVersion const& iv) noexcept {
			assert(Exists(iv));
			return buf[iv.index].value;
		}

		IndexType Left() const {
			return cap - len + freeCount;
		}

		IndexType Count() const {
			return len - freeCount;
		}

		[[nodiscard]] bool Empty() const {
			return len - freeCount == 0;
		}

		// ll.Foreach( [&](auto& o) { o..... } );
		// ll.Foreach( [&](auto& o)->bool { if ( o.... ) return ... } );
		template<typename F>
		void Foreach(F&& f, IndexType beginIdx = -1) {
            if (beginIdx == -1) {
                beginIdx = head;
            }
			if constexpr (std::is_void_v<decltype(f(buf[0].value))>) {
				for (auto idx = beginIdx; idx != -1; idx = Next(idx)) {
					f(buf[idx].value);
				}
			} else {
				for (IndexType next, idx = beginIdx; idx != -1;) {
					next = Next(idx);
					if (f(buf[idx].value)) {
						Remove(idx);
					}
					idx = next;
				}
			}
		}

		// ll.ForeachReverse( [&](auto& o) { o..... } );
		// ll.ForeachReverse( [&](auto& o)->bool { if ( o.... ) return ... } );
		template<typename F>
		void ForeachReverse(F&& f) {
			if constexpr (std::is_void_v<decltype(f(buf[0].value))>) {
				for (auto idx = tail; idx != -1; idx = Prev(idx)) {
					f(buf[idx].value);
				}
			} else {
				for (IndexType prev, idx = tail; idx != -1;) {
					prev = Prev(idx);
					if (f(buf[idx].value)) {
						Remove(idx);
					}
					idx = prev;
				}
			}
		}
	};

    template<typename T, typename IndexType, typename VersionType>
    struct IsPod<ListDoubleLink<T, IndexType, VersionType>> : std::true_type {};
}


/*
int main() {
	int counter, n = 1, m = 5000000;
	auto secs = xx::NowEpochSeconds();

	counter = 0;
	for (size_t i = 0; i < n; i++) {

		xx::ListLink<int, int> ll;
		ll.Reserve(m);
		for (size_t j = 1; j <= m; j++) {
			ll.Emplace(j);
		}

		ll.Foreach([](auto& o)->bool {
			return o == 2 || o == 4;
			});
		ll.Emplace(2);
		ll.Emplace(4);

		ll.Foreach([&](auto& o) {
			counter += o;
			});
	}

	xx::CoutN("ListLink counter = ", counter, " secs = ", xx::NowEpochSeconds(secs));

	counter = 0;
	for (size_t i = 0; i < n; i++) {

		xx::ListDoubleLink<int, int, int> ll;
		ll.Reserve(m);
		for (size_t j = 1; j <= m; j++) {
			ll.Emplace(j);
		}

		ll.Foreach([](auto& o)->bool {
			return o == 2 || o == 4;
			});
		ll.Emplace(2);
		ll.Emplace(4);

		ll.Foreach([&](auto& o) {
			counter += o;
			});
	}

	xx::CoutN("ListDoubleLink counter = ", counter, " secs = ", xx::NowEpochSeconds(secs));

	counter = 0;
	for (size_t i = 0; i < n; i++) {

		std::map<int, int> ll;
		int autoInc{};
		for (size_t j = 1; j <= m; j++) {
			ll[++autoInc] = j;
		}

		for (auto it = ll.begin(); it != ll.end();) {
			if (it->second == 2 || it->second == 4) {
				it = ll.erase(it);
			} else {
				++it;
			}
		}
		ll[++autoInc] = 2;
		ll[++autoInc] = 4;

		for (auto&& kv : ll) {
			counter += kv.second;
		}
	}

	xx::CoutN("map counter = ", counter, " secs = ", xx::NowEpochSeconds(secs));


	counter = 0;
	for (size_t i = 0; i < n; i++) {

		std::list<int> ll;
		for (size_t j = 1; j <= m; j++) {
			ll.push_back(j);
		}

		for (auto it = ll.begin(); it != ll.end();) {
			if (*it == 2 || *it == 4) {
				it = ll.erase(it);
			} else {
				++it;
			}
		}
		ll.push_back(2);
		ll.push_back(4);

		for (auto&& v : ll) {
			counter += v;
		}
	}

	xx::CoutN("list counter = ", counter, " secs = ", xx::NowEpochSeconds(secs));


	counter = 0;
	for (size_t i = 0; i < n; i++) {

		std::pmr::list<int> ll;
		for (size_t j = 1; j <= m; j++) {
			ll.push_back(j);
		}

		for (auto it = ll.begin(); it != ll.end();) {
			if (*it == 2 || *it == 4) {
				it = ll.erase(it);
			} else {
				++it;
			}
		}
		ll.push_back(2);
		ll.push_back(4);

		for (auto&& v : ll) {
			counter += v;
		}
	}

	xx::CoutN("pmr list counter = ", counter, " secs = ", xx::NowEpochSeconds(secs));

	return 0;

	{
		struct Scene;
		struct Monster;
		using Monsters = std::list<std::shared_ptr<Monster>>;

		struct Monster {
			Scene* scene;
			Monsters::iterator iter;
			std::weak_ptr<Monster> target;
			Monster(Scene* const& scene, Monsters::iterator const& iter) : scene(scene), iter(iter) {}
			bool Update() { return true; }
			void Draw() {}
		};

		struct Scene {
			Monsters monsters;
			void Run() {
				for (size_t i = 0; i < 100; i++) {
					auto& m = monsters.emplace_back(std::make_shared<Monster>(this, Monsters::iterator{} ));
					m->iter = monsters.rbegin().base();
				}
				for (auto&& iter = monsters.rbegin(); iter != monsters.rend();) {
					if ( (*iter)->Update() ) {
						iter = Monsters::reverse_iterator(monsters.erase(iter.base()));
					} else {
						++iter;
					}
				}
				for (auto&& m : monsters) {
					m->Draw();
				}
			}
		};

		Scene scene;
		scene.Run();
	}


	{
		struct Scene;
		struct Monster;
		using Monsters = xx::ListDoubleLink<xx::Shared<Monster>>;

		struct Monster {
			Scene* scene;
			Monsters::IndexAndVersion iter;
			xx::Weak<Monster> target;
			bool Update() { return true; }
			void Draw() {}
		};

		struct Scene {
			Monsters monsters;
			void Run() {
				for (size_t i = 0; i < 100; i++) {
					monsters.Emplace().Emplace(this, monsters.Tail());
				}
				monsters.ForeachReverse([](auto& m)->bool {
					return m->Update();
				});
				monsters.Foreach([](auto& m) {
					m->Draw();
				});
			}
		};

		Scene scene;
		scene.Run();
	}


	{
		struct Monster;
		using Monsters = xx::ListDoubleLink<Monster>;
		using MonsterIV = Monsters::IndexAndVersion;

		struct Scene;
		struct Monster {
			Scene* scene;
			MonsterIV iv, target{};
			bool Update() { return true; }
			void Draw() {}
		};

		struct Scene {
			Monsters monsters;
			void Run() {
				for (size_t i = 0; i < 100; i++) {
					monsters.Add()(this, monsters.Tail());
				}
				monsters.ForeachReverse([](auto& m)->bool {
					return m.Update();
				});
				monsters.Foreach([](auto& m) {
					m.Draw();
				});
			}
		};

		Scene scene;
		scene.Run();
	}

	return 0;
}
*/