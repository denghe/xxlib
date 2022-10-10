#pragma once
#include <memory>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace xx {

	// 简单数据类型容器( 定长 buf 性质 )
	template<typename T = char, bool useNewDelete = true>
	struct PodContainer {
		T* buf;
		size_t cap;

		PodContainer(PodContainer const& o) = delete;
		PodContainer& operator=(PodContainer const& o) = delete;

		PodContainer(size_t const& cap_ = 0) : buf(cap_ == 0 ? nullptr : Alloc(cap_)), cap(cap_) {}
		~PodContainer() {
			if (buf) {
				if constexpr (useNewDelete) {
					delete[](char*)buf;
				}
				else {
					free(buf);
				}
				buf = nullptr;
			}
		}
		PodContainer(PodContainer&& o) noexcept : buf(o.buf), cap(o.cap) {
			o.buf = nullptr;
			o.cap = 0;
		}
		PodContainer& operator=(PodContainer&& o) noexcept {
			buf = o.buf;
			cap = o.cap;
			o.buf = nullptr;
			o.cap = 0;
			return *this;
		}

		static T* Alloc(size_t const& count) {
			if constexpr (useNewDelete) return (T*)new char[count * sizeof(T)];
			else return (T*)malloc(count * sizeof(T));
		}

		operator T*& () {
			return buf;
		}
		operator T* const& () const {
			return buf;
		}

		T& operator[](size_t const& i) {
			assert(i < cap);
			return buf[i];
		}
		T const& operator[](size_t const& i) const {
			assert(i < cap);
			return buf[i];
		}
	};

	// 方便用来替代裸 buf, 不初始化内存, 实现自动释放
	using CharBuf = PodContainer<char, false>;
	using UCharBuf = PodContainer<uint8_t, false>;

	// 类似 std::vector 的专用于放 pod 或 非严格pod( 是否执行构造，析构 无所谓 ) 类型的容器，可拿走 buf
	// useNewDelete == false: malloc + free
	template<typename T, bool useNewDelete = true>
	struct PodVector : PodContainer<T, useNewDelete> {
		using Base = PodContainer<T, useNewDelete>;
		size_t len;

		PodVector(PodVector const& o) = delete;
		PodVector& operator=(PodVector const& o) = delete;
		explicit PodVector(size_t const& cap_ = 0, size_t const& len_ = 0) : Base(cap_), len(len_) {}

		PodVector(PodVector&& o) noexcept : Base(std::move(o)), len(o.len) {
			o.len = 0;
		}
		PodVector& operator=(PodVector&& o) noexcept {
			Base::operator=(std::move(o));
			len = o.len;
			o.len = 0;
			return *this;
		}

		void Reserve(size_t const& cap_) {
			if (cap_ <= this->cap) return;
			if (!this->cap) {
				this->cap = std::max(cap_, 4096 / sizeof(T));
			}
			else do {
				this->cap += this->cap;
			} while (this->cap < cap_);

			if constexpr (useNewDelete) {
				auto newBuf = (T*)new char[this->cap * sizeof(T)];
				memcpy(newBuf, this->buf, len * sizeof(T));
				delete[](char*)this->buf;
				this->buf = newBuf;
			}
			else {
				this->buf = (T*)realloc(this->buf, this->cap * sizeof(T));
			}
		}

		void Resize(size_t const& len_) {
			if (len_ > len) {
				Reserve(len_);
			}
			len = len_;
		}

		T* TakeAwayBuf() noexcept {
			auto r = this->buf;
			this->buf = nullptr;
			this->cap = 0;
			len = 0;
			return r;
		}

		template<bool needReserve = true, typename...Args>
		T& Emplace(Args&&...args) noexcept {
			if constexpr (needReserve) {
				Reserve(len + 1);
			}
			return *new (&this->buf[len++]) T(std::forward<Args>(args)...);
		}


		T* begin() const noexcept { return buf; }
		T* end() const noexcept { return buf + len; }
	};
}
