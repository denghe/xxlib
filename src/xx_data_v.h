#pragma once

#include <utility>
#include <cassert>
#include <cstdint>

#ifndef XX_NOINLINE
#ifndef NDEBUG
#define XX_NOINLINE
#define XX_FORCE_INLINE
#else
#ifdef _MSC_VER
#define XX_NOINLINE __declspec(noinline)
#define XX_FORCE_INLINE __forceinline
#else
#define XX_NOINLINE __attribute__((noinline))
#define XX_FORCE_INLINE __attribute__((always_inline))
#endif
#endif
#endif

namespace xx {

    // 基础二进制数据容器( 视图只读版 )( 带 xx::Data 的 Read 系列函数 )
    struct Data_v {
        uint8_t *buf;
        size_t len;
        size_t offset;

        Data_v()
                : buf(nullptr)
                , len(0)
                , offset(0)
        {
        }

        // 引用一段数据 构造
        [[maybe_unused]] Data_v(void const *const &buf, size_t const &len, size_t const &offset = 0)
            : buf((uint8_t *)buf)
            , len(len)
            , offset(offset)
        {
        }

        [[maybe_unused]] void Reset(void const *const &buf_, size_t const &len_, size_t const &offset_ = 0) {
            buf = (uint8_t *)buf_;
            len = len_;
            offset = offset_;
        }

        Data_v(Data_v const &o) = default;
        Data_v &operator=(Data_v const &o) = default;

        // 判断数据是否一致
        XX_FORCE_INLINE bool operator==(Data_v const &o) {
            if (&o == this) return true;
            if (len != o.len) return false;
            return 0 == memcmp(buf, o.buf, len);
        }

        XX_FORCE_INLINE bool operator!=(Data_v const &o) {
            return !this->operator==(o);
        }

        // 下标访问
        XX_FORCE_INLINE uint8_t const &operator[](size_t const &idx) const {
            assert(idx < len);
            return buf[idx];
        }

        /***************************************************************************************************************************/

        // 读指定长度 buf 到 tar. 返回非 0 则读取失败
        XX_FORCE_INLINE [[nodiscard]] int ReadBuf(void *const &tar, size_t const &siz) {
            if (offset + siz > len) return __LINE__;
            memcpy(tar, buf + offset, siz);
            offset += siz;
            return 0;
        }

        // 定长读. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_pod_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE [[nodiscard]] int ReadFixed(T &v) {
            if constexpr (sizeof(T) == 1) {
                if (offset == len) return __LINE__;
                v = *(T *) (buf + offset);
                ++offset;
                return 0;
            } else {
                return ReadBuf(&v, sizeof(T));
            }
        }

        // 从指定下标读指定长度. 不改变 offset. 返回非 0 则读取失败
        [[maybe_unused]] XX_FORCE_INLINE [[nodiscard]] int ReadBufAt(size_t const &idx, void *const &tar, size_t const &siz) const {
            if (idx + siz >= len) return __LINE__;
            memcpy(tar, buf + idx, siz);
            return 0;
        }

        // 从指定下标定长读. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_pod_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE [[nodiscard]] int ReadFixedAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) >= len) return __LINE__;
            if constexpr (sizeof(T) == 1) {
                v = *(T *) (buf + idx);
            } else {
                memcpy(&v, buf + idx, sizeof(T));
            }
            return 0;
        }

        // 变长读. 返回非 0 则读取失败
        template<typename T>
        [[maybe_unused]] XX_FORCE_INLINE [[nodiscard]] int ReadVarInteger(T &v) {
            using UT = std::make_unsigned_t<T>;
            UT u(0);
            for (size_t shift = 0; shift < sizeof(T) * 8; shift += 7) {
                if (offset == len) return __LINE__;
                auto b = (UT) buf[offset++];
                u |= UT((b & 0x7Fu) << shift);
                if ((b & 0x80) == 0) {
                    if constexpr (std::is_signed_v<T>) {
                        v = ZigZagDecode(u);
                    } else {
                        v = u;
                    }
                    return 0;
                }
            }
            return __LINE__;
        }

    };
}
