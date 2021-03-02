#pragma once

#include <initializer_list>
#include <utility>
#include <cassert>
#include <cstdint>

#ifdef _WIN32

#include <intrin.h>     // _BitScanReverse[64]
#include <objbase.h>

#endif

#ifndef XX_NOINLINE
#   ifndef NDEBUG
#       define XX_NOINLINE
#       define XX_FORCE_INLINE
#   else
#       ifdef _MSC_VER
#           define XX_NOINLINE __declspec(noinline)
#           define XX_FORCE_INLINE __forceinline
#       else
#           define XX_NOINLINE __attribute__((noinline))
#           define XX_FORCE_INLINE __attribute__((always_inline))
#       endif
#   endif
#endif

#ifndef _WIN32
#include <arpa/inet.h>
#endif
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define __LITTLE_ENDIAN__
#   elif __BYTE_ORDER == __BIG_ENDIAN
#       define __BIG_ENDIAN__
#   elif _WIN32
#       define __LITTLE_ENDIAN__
#   endif
#endif

#define XX_DATA_BE_LE_COPY( a, b, T ) \
if constexpr(sizeof(T) == 1) { \
    a[0] = b[0]; \
} \
if constexpr(sizeof(T) == 2) { \
    a[0] = b[1]; \
    a[1] = b[0]; \
} \
if constexpr(sizeof(T) == 4) { \
    a[0] = b[3]; \
    a[1] = b[2]; \
    a[2] = b[1]; \
    a[3] = b[0]; \
} \
if constexpr(sizeof(T) == 8) { \
    a[0] = b[7]; \
    a[1] = b[6]; \
    a[2] = b[5]; \
    a[3] = b[4]; \
    a[4] = b[3]; \
    a[5] = b[2]; \
    a[6] = b[1]; \
    a[7] = b[0]; \
}

namespace xx {

    // 基础二进制数据容器. 可简单读写( 当前不支持大尾 )，可配置预留长度( 方便有些操作在 buf 最头上放东西 )
    template<size_t reserveLen = 0>
    struct Data {
        uint8_t *buf = nullptr;
        size_t len = 0;
        size_t cap = 0;
        size_t offset = 0;

        // 预分配空间 构造
        [[maybe_unused]] explicit Data(size_t const &newCap) {
            assert(newCap);
            auto siz = Round2n(reserveLen + newCap);
            buf = ((uint8_t *) malloc(siz)) + reserveLen;
            cap = siz - reserveLen;
        }

        // 通过 复制一段数据 来构造
        [[maybe_unused]] Data(void const *const &ptr, size_t const &siz) {
            WriteBuf(ptr, siz);
        }

        Data() = default;

        Data(Data const &o) {
            operator=(o);
        }

        Data(Data &&o) noexcept {
            operator=(std::move(o));
        }

        XX_FORCE_INLINE Data &operator=(Data const &o) {
            if (this == &o) return *this;
            Clear();
            WriteBuf(o.buf, o.len);
            return *this;
        }

        XX_FORCE_INLINE Data &operator=(Data &&o) noexcept {
            std::swap(buf, o.buf);
            std::swap(len, o.len);
            std::swap(cap, o.cap);
            std::swap(offset, o.offset);
            return *this;
        }

        // 判断数据是否一致
        XX_FORCE_INLINE bool operator==(Data const &o) {
            if (&o == this) return true;
            if (len != o.len) return false;
            return 0 == memcmp(buf, o.buf, len);
        }

        XX_FORCE_INLINE bool operator!=(Data const &o) {
            return !this->operator==(o);
        }

        // 确保空间足够
        template<bool CheckCap = true>
        XX_NOINLINE void Reserve(size_t const &newCap) {
            if (CheckCap && newCap <= cap) return;

            auto siz = Round2n(reserveLen + newCap);
            auto newBuf = ((uint8_t *) malloc(siz)) + reserveLen;
            memcpy(newBuf, buf, len);

            // 这里判断 cap 不判断 buf, 是因为 gcc 优化会导致 if 失效, 无论如何都会执行 free
            if (cap) {
                free(buf - reserveLen);
            }
            buf = newBuf;
            cap = siz - reserveLen;
        }

        // 返回旧长度
        XX_FORCE_INLINE size_t Resize(size_t const &newLen) {
            if (newLen > len) {
                Reserve<false>(newLen);
            }
            auto rtv = len;
            len = newLen;
            return rtv;
        }


        // 通过 初始化列表 填充内容. 填充前会先 Clear. 用法: d.Fill({ 1,2,3. ....})
        template<typename T = int32_t, typename = std::enable_if_t<std::is_convertible_v<T, uint8_t>>>
        [[maybe_unused]] XX_FORCE_INLINE void Fill(std::initializer_list<T> const &bytes) {
            Clear();
            Reserve(bytes.size());
            for (auto &&b : bytes) {
                buf[len++] = (uint8_t) b;
            }
        }


        // 下标访问
        XX_FORCE_INLINE uint8_t &operator[](size_t const &idx) {
            assert(idx < len);
            return buf[idx];
        }

        XX_FORCE_INLINE uint8_t const &operator[](size_t const &idx) const {
            assert(idx < len);
            return buf[idx];
        }


        // 从头部移除指定长度数据( 常见于拆包处理移除掉已经访问过的包数据, 将残留部分移动到头部 )
        XX_FORCE_INLINE [[maybe_unused]] void RemoveFront(size_t const &siz) {
            assert(siz <= len);
            if (!siz) return;
            len -= siz;
            if (len) {
                memmove(buf, buf + siz, len);
            }
        }

        /***************************************************************************************************************************/



        // 追加写入一段 buf
        template<bool needReserve = true>
        XX_FORCE_INLINE void WriteBuf(void const *const &ptr, size_t const &siz) {
            if constexpr (needReserve) {
                if (len + siz > cap) {
                    Reserve<false>(len + siz);
                }
            }
            memcpy(buf + len, ptr, siz);
            len += siz;
        }

        // 在指定 idx 写入一段 buf
        [[maybe_unused]] XX_FORCE_INLINE void WriteBufAt(size_t const &idx, void const *const &ptr, size_t const &siz) {
            if (idx + siz > len) {
                Resize(siz + idx);
            }
            memcpy(buf + idx, ptr, siz);
        }



        // 追加写入 float / double / integer ( 定长 Little Endian )
        template<typename T, bool needReserve = true, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixed(T const &v) {
            if constexpr (needReserve) {
                if (len + 1 > cap) {
                    Reserve<false>(len + 1);
                }
            }

#ifdef __BIG_ENDIAN__
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(buf + len, &v, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( (buf + len),  ((uint8_t *)&v), T )
            }
#else
            memcpy(buf + len, &v, sizeof(T));
#endif
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Little Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedAt(size_t const &idx, T const &v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __BIG_ENDIAN__
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(buf + len, &v, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( (buf + idx),  ((uint8_t *)&v), T )
            }
#else
            memcpy(buf + len, &v, sizeof(T));
#endif
        }

        // 追加写入 float / double / integer ( 定长 Big Endian )
        template<typename T, bool needReserve = true, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedBE(T const &v) {
            if constexpr (needReserve) {
                if (len + sizeof(T) > cap) {
                    Reserve<false>(len + sizeof(T));
                }
            }
#ifdef __BIG_ENDIAN__
            memcpy(buf + len, &v, sizeof(T));
#else
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(buf + len, &v, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( (buf + len),  ((uint8_t *)&v), T )
            }
#endif
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Big Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedBEAt(size_t const &idx, T const &v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __BIG_ENDIAN__
            memcpy(&v, buf + idx, sizeof(T));
#else
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(buf + len, &v, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( (buf + idx),  ((uint8_t *)&v), T )
            }
#endif
        }



        // 追加写入整数( 7bit 变长格式 )
        template<typename T, bool needReserve = true, typename = std::enable_if_t<std::is_integral_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteVarInteger(T const &v) {
            using UT = std::make_unsigned_t<T>;
            UT u(v);
            if constexpr (std::is_signed_v<T>) {
                u = ZigZagEncode(v);
            }
            if constexpr (needReserve) {
                if (len + sizeof(T) + 1 > cap) {
                    Reserve<false>(len + sizeof(T) + 1);
                }
            }
            while (u >= 1 << 7) {
                buf[len++] = uint8_t((u & 0x7fu) | 0x80u);
                u = UT(u >> 7);
            }
            buf[len++] = uint8_t(u);
        }



        // 跳过指定长度字节数不写。返回起始 len
        template<bool needReserve = true>
        [[maybe_unused]] XX_FORCE_INLINE size_t WriteJump(size_t const &siz) {
            auto bak = len;
            if constexpr (needReserve) {
                if (len + siz > cap) {
                    Reserve<false>(len + siz);
                }
            }
            len += siz;
            return bak;
        }

        /***************************************************************************************************************************/

        // 读 定长buf 到 tar. 返回非 0 则读取失败
        XX_FORCE_INLINE [[nodiscard]] int ReadBuf(void *const &tar, size_t const &siz) {
            if (offset + siz > len) return __LINE__;
            memcpy(tar, buf + offset, siz);
            offset += siz;
            return 0;
        }

        // 从指定下标 读 定长buf. 不改变 offset. 返回非 0 则读取失败
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadBufAt(size_t const &idx, void *const &tar, size_t const &siz) const {
            if (idx + siz >= len) return __LINE__;
            memcpy(tar, buf + idx, siz);
            return 0;
        }



        // 读 定长小尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadFixed(T &v) {
            if constexpr (sizeof(T) == 1) {
                if (offset == len) return __LINE__;
                v = *(T *) (buf + offset);
                ++offset;
                return 0;
            } else {
                return ReadBuf(&v, sizeof(T));
            }
        }

        // 从指定下标 读 定长小尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadFixedAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) >= len) return __LINE__;
            if constexpr (sizeof(T) == 1) {
                v = *(T *) (buf + idx);
            } else {
                memcpy(&v, buf + idx, sizeof(T));
            }
            return 0;
        }


        // 读 定长大尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadFixedBE(T &v) {
            if (offset + sizeof(T) >= len) return __LINE__;
#ifdef __BIG_ENDIAN__
            memcpy(&v, buf + offset, sizeof(T));
#else
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(&v, buf + offset, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( ((uint8_t *)&v), (buf + offset),  T )
            }
#endif
            return 0;
        }

        // 从指定下标 读 定长大尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadFixedBEAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) >= len) return __LINE__;
#ifdef __BIG_ENDIAN__
            memcpy(&v, buf + offset, sizeof(T));
#else
            if constexpr(std::is_floating_point_v<T>) {
                memcpy(&v, buf + idx, sizeof(T));
            }
            else {
                XX_DATA_BE_LE_COPY( ((uint8_t *)&v), (buf + idx),  T )
            }
#endif
            return 0;
        }


        // 读 变长整数. 返回非 0 则读取失败
        template<typename T>
        [[maybe_unused]] XX_FORCE_INLINE[[nodiscard]] int ReadVarInteger(T &v) {
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


        /***************************************************************************************************************************/

        ~Data() {
            Clear(true);
        }

        // len 清 0, 可彻底释放 buf
        XX_FORCE_INLINE void Clear(bool const &freeBuf = false) {
            if (freeBuf && cap) {
                free(buf - reserveLen);
                buf = nullptr;
                cap = 0;
            }
            len = 0;
            offset = 0;
        }

        // 计算内存对齐的工具函数
        inline static size_t Calc2n(size_t const &n) noexcept {
            assert(n);
#ifdef _MSC_VER
            unsigned long r = 0;
#if defined(_WIN64) || defined(_M_X64)
            _BitScanReverse64(&r, n);
# else
            _BitScanReverse(&r, n);
# endif
            return (size_t) r;
#else
#if defined(__LP64__) || __WORDSIZE == 64
            return int(63 - __builtin_clzl(n));
# else
            return int(31 - __builtin_clz(n));
# endif
#endif
        }

        // 返回一个刚好大于 n 的 2^x 对齐数
        inline static size_t Round2n(size_t const &n) noexcept {
            auto rtv = size_t(1) << Calc2n(n);
            if (rtv == n) return n;
            else return rtv << 1;
        }
    };
}
