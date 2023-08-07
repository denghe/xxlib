#pragma once
#include "xx_typetraits.h"

namespace xx {

	// fast add/remove container. can visit members by Add order ( faster than map 10+ times )
	// Example at the xx_listdoublelink.h

	template<typename T, typename SizeType = ptrdiff_t, SizeType initCap = 64>
	struct ListLink {

		struct Node {
			SizeType next;
			T value;
		};

		Node* buf{};
		SizeType cap{}, len{};
		SizeType head{ -1 }, tail{ -1 }, freeHead{ -1 }, freeCount{};

		ListLink() = default;
		ListLink(ListLink const&) = delete;
		ListLink& operator=(ListLink const&) = delete;
		ListLink(ListLink&& o) noexcept {
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
		ListLink& operator=(ListLink&& o) noexcept {
			std::swap(buf, o.buf);
			std::swap(cap, o.cap);
			std::swap(len, o.len);
			std::swap(head, o.head);
			std::swap(tail, o.tail);
			std::swap(freeHead, o.freeHead);
			std::swap(freeCount, o.freeCount);
			return *this;
		}
		~ListLink() {
			Clear<true>();
		}

		void Reserve(SizeType const& newCap) noexcept {
			assert(newCap > 0);
			if (newCap <= cap) return;
			cap = newCap;
			if constexpr (IsPod_v<T>) {
				buf = (Node*)realloc(buf, sizeof(Node) * newCap);
			} else {
				auto newBuf = (Node*)::malloc(newCap * sizeof(Node));
				for (SizeType i = 0; i < len; ++i) {
					newBuf[i].next = buf[i].next;
					new (&newBuf[i].value) T((T&&)buf[i].value);
					buf[i].value.~T();
				}
				::free(buf);
				buf = newBuf;
			}
		}

		//auto&& o = new (&ll.AddCore()) T( ... );
		[[nodiscard]] T& AddCore() {
			SizeType idx;
			if (freeCount > 0) {
				idx = freeHead;
				freeHead = buf[idx].next;
				freeCount--;
			} else {
				if (len == cap) {
					if constexpr (initCap <= 0) {
						throw std::logic_error("Memory usage has reached the maximum limit!");
					} else {
						Reserve(cap ? cap * 2 : initCap);
					}
				}
				idx = len;
				len++;
			}

			buf[idx].next = -1;

			if (tail >= 0) {
				buf[tail].next = idx;
				tail = idx;
			} else {
				assert(head == -1);
				head = tail = idx;
			}

			return buf[idx].value;
		}

		// auto& o = Add()( ... );
		template<typename U = T>
		[[nodiscard]] TCtor<U> Add() {
			return { (U*)&AddCore() };
		}

		template<typename U = T, typename...Args>
		U& Emplace(Args&&...args) {
			return *new (&AddCore()) U(std::forward<Args>(args)...);
		}

		// return next index
		SizeType Remove(SizeType const& idx, SizeType const& prevIdx = -1) {
			assert(idx >= 0);
			assert(idx < len);

			if (idx == head) {
				head = buf[idx].next;
			}
			if (idx == tail) {
				tail = prevIdx;
			}
			auto r = buf[idx].next;
			if (prevIdx >= 0) {
				buf[prevIdx].next = r;
			}
			buf[idx].value.~T();
			buf[idx].next = freeHead;
			freeHead = idx;
			++freeCount;
			return r;
		}

		SizeType Next(SizeType const& idx) const {
			return buf[idx].next;
		}

		template<bool freeBuf = false>
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
		}

		T const& operator[](SizeType const& idx) const noexcept {
			assert(idx >= 0);
			assert(idx < len);
			return buf[idx].value;
		}

		T& operator[](SizeType const& idx) noexcept {
			assert(idx >= 0);
			assert(idx < len);
			return buf[idx].value;
		}

		SizeType Left() const {
			return cap - len + freeCount;
		}

		SizeType Count() const {
			return len - freeCount;
		}

		[[nodiscard]] bool Empty() const {
			return len - freeCount == 0;
		}

		// ll.Foreach( [&](auto& o) { o..... } );
		// ll.Foreach( [&](auto& o)->bool { if ( o.... ) return ... } );
		template<typename F>
		void Foreach(F&& f) {
			if constexpr (std::is_void_v<decltype(f(buf[0].value))>) {
				for (auto idx = head; idx != -1; idx = Next(idx)) {
					f(buf[idx].value);
				}
			} else {
				for (SizeType prev = -1, next, idx = head; idx != -1;) {
					if (f(buf[idx].value)) {
						next = Remove(idx, prev);
					} else {
						next = Next(idx);
						prev = idx;
					}
					idx = next;
				}
			}
		}

		// ll.FindIf( [&](auto& o)->bool { if ( o.... ) return ... } );
		template<typename F>
		std::pair<SizeType, SizeType> FindIf(F&& f) {
			for (SizeType prev = -1, next, idx = head; idx != -1;) {
				if (f(buf[idx].value)) return {idx, prev};
				else {
					next = Next(idx);
					prev = idx;
				}
				idx = next;
			}
			return { -1, -1 };
		}
	};

    template<typename T, typename SizeType, SizeType initCap>
    struct IsPod<ListLink<T, SizeType, initCap>> : std::true_type {};
}
