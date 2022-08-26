#pragma once
#include "xx_data_funcs.h"

namespace xx {

	// 从 Data_rw<sizeof(size_t)*2> 的 buf move 数据出来，并在最前面填充 数据长度 + 引用计数, 实现一个简易的智能指针. 当引用计数 == 0 时，free buf
	struct DataShared {

		struct Header {
			size_t len;
			size_t numRefs;
		};
		static constexpr size_t hLen = Data::GetBufHeaderReserveLen();
		static_assert(hLen >= sizeof(Header));

		Header* h;

		DataShared(Data&& d) {
			if (d) {
				h = (Header*)(d.buf - hLen);
				h->len = d.len;
				h->numRefs = 1;
				d.Reset();
			}
			else {
				h = nullptr;
			}
		}

		DataShared(DataShared const& ds)
			: h(ds.h) {
			if (h) {
				assert(h->numRefs > 0);
				++h->numRefs;
			}
		}

		DataShared(DataShared&& ds) noexcept
			: h(ds.h) {
			ds.h = nullptr;
		}

		DataShared& operator=(DataShared const& ds) {
			if (h == ds.h) return *this;
			Clear();
			h = ds.h;
			if (h) {
				assert(h->numRefs > 0);
				++h->numRefs;
			}
			return *this;
		}

		DataShared& operator=(DataShared&& ds) noexcept {
			std::swap(h, ds.h);
			return *this;
		}

		void Clear() {
			if (h) {
				if (--h->numRefs == 0) {
					free(h);
				}
				h = nullptr;
			}
		}

		~DataShared() {
			Clear();
		}

		uint8_t* GetBuf() const {
			if (h) return (uint8_t*)h + hLen;
			else return nullptr;
		}

		size_t GetLen() const {
			if (h) return h->len;
			else return 0;
		}

		size_t GetNumRefs() const {
			if (h) return h->numRefs;
			else return 0;
		}
	};
}
