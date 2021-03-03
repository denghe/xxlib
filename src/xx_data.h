#pragma once

#include <initializer_list>
#include <utility>
#include <cassert>
#include <cstdint>
#include <cstring>      // memcmp

#ifdef _WIN32
#include <intrin.h>     // _BitScanReverse[64] _byteswap_ulong _byteswap_uint64
#include <objbase.h>
#else

#include <arpa/inet.h>  // __BYTE_ORDER __LITTLE_ENDIAN __BIG_ENDIAN

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


#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define __LITTLE_ENDIAN__
#   elif __BYTE_ORDER == __BIG_ENDIAN
#       define __BIG_ENDIAN__
#   elif _WIN32
#       define __LITTLE_ENDIAN__
#   endif
#endif

namespace xx {

    // 基础二进制数据容器( 视图只读版 )( 带 xx::Data 的 Read 系列函数 )
    struct Data_v {
        uint8_t *buf;
        size_t len;
        size_t offset;

        Data_v()
                : buf(nullptr), len(0), offset(0) {
        }

        // 引用一段数据 构造
        [[maybe_unused]] Data_v(void const *const &buf, size_t const &len, size_t const &offset = 0)
                : buf((uint8_t *) buf), len(len), offset(offset) {
        }

        [[maybe_unused]] void Reset(void const *const &buf_, size_t const &len_, size_t const &offset_ = 0) {
            buf = (uint8_t *) buf_;
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

        // 数字字节序交换( 浮点例外 )
        template<typename T>
        XX_FORCE_INLINE static T BSwap(T const &i) {
            if constexpr(std::is_floating_point_v<T>) return i;
            if constexpr(sizeof(T) == 2) return (*(uint16_t *) &i >> 8) | (*(uint16_t *) &i << 8);
#ifdef _WIN32
            if constexpr(sizeof(T) == 4) return (T) _byteswap_ulong(*(uint32_t *) &i);
            if constexpr(sizeof(T) == 8) return (T) _byteswap_uint64(*(uint64_t *) &i);
#else
            if constexpr(sizeof(T) == 4) return (T) __builtin_bswap32(*(uint32_t *) &i);
            if constexpr(sizeof(T) == 8) return (T) __builtin_bswap64(*(uint64_t *) &i);
#endif
            return i;
        }

        /***************************************************************************************************************************/

        // 读 定长buf 到 tar. 返回非 0 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadBuf(void *const &tar, size_t const &siz) {
            if (offset + siz > len) return __LINE__;
            memcpy(tar, buf + offset, siz);
            offset += siz;
            return 0;
        }

        // 从指定下标 读 定长buf. 不改变 offset. 返回非 0 则读取失败
        [[maybe_unused]]  [[nodiscard]] XX_FORCE_INLINE int ReadBufAt(size_t const &idx, void *const &tar, size_t const &siz) const {
            if (idx + siz > len) return __LINE__;
            memcpy(tar, buf + idx, siz);
            return 0;
        }

        // 读 定长小尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadFixed(T &v) {
            if (offset + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + offset, sizeof(T));
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长小尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadFixedAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            return 0;
        }

        // 读 定长大尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadFixedBE(T &v) {
            if (offset + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + offset, sizeof(T));
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长大尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadFixedBEAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) >= len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            return 0;
        }

        // 读 变长整数. 返回非 0 则读取失败
        template<typename T>
        [[maybe_unused]] [[nodiscard]] XX_FORCE_INLINE int ReadVarInteger(T &v) {
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


    /***************************************************************************************************************************/
    /***************************************************************************************************************************/


    // 基础二进制数据容器. 可简单读写( 当前不支持大尾 )，可配置预留长度( 方便有些操作在 buf 最头上放东西 )
    template<size_t reserveLen = 0>
    struct Data : Data_v {
        size_t cap = 0;

        // 预分配空间 构造
        [[maybe_unused]] explicit Data(size_t const &cap)
                : cap(cap) {
            assert(cap);
            auto siz = Round2n(reserveLen + cap);
            buf = (new uint8_t[siz]) + reserveLen;
            this->cap = siz - reserveLen;
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
            return ((Data_v *) this)->operator==(*(Data_v *) &o);
        }

        XX_FORCE_INLINE bool operator!=(Data const &o) {
            return !this->operator==(o);
        }

        // 确保空间足够
        template<bool CheckCap = true>
        XX_NOINLINE void Reserve(size_t const &newCap) {
            if (CheckCap && newCap <= cap) return;

            auto siz = Round2n(reserveLen + newCap);
            auto newBuf = (new uint8_t[siz]) + reserveLen;
            memcpy(newBuf, buf, len);

            // 这里判断 cap 不判断 buf, 是因为 gcc 优化会导致 if 失效, 无论如何都会执行 free
            if (cap) {
                delete[](buf - reserveLen);
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


        // 下标可写访问
        XX_FORCE_INLINE uint8_t &operator[](size_t const &idx) {
            assert(idx < len);
            return buf[idx];
        }

        // 从头部移除指定长度数据( 常见于拆包处理移除掉已经访问过的包数据, 将残留部分移动到头部 )
        [[maybe_unused]] XX_FORCE_INLINE void RemoveFront(size_t const &siz) {
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
                Resize(idx + siz);
            }
            memcpy(buf + idx, ptr, siz);
        }


        // 追加写入 float / double / integer ( 定长 Little Endian )
        template<typename T, bool needReserve = true, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixed(T v) {
            if constexpr (needReserve) {
                if (len + sizeof(T) > cap) {
                    Reserve<false>(len + sizeof(T));
                }
            }
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + len, &v, sizeof(T));
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Little Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedAt(size_t const &idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + idx, &v, sizeof(T));
        }

        // 追加写入 float / double / integer ( 定长 Big Endian )
        template<typename T, bool needReserve = true, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedBE(T v) {
            if constexpr (needReserve) {
                if (len + sizeof(T) > cap) {
                    Reserve<false>(len + sizeof(T));
                }
            }
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + len, &v, sizeof(T));
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Big Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        [[maybe_unused]] XX_FORCE_INLINE void WriteFixedBEAt(size_t const &idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + idx, &v, sizeof(T));
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
                if (len + sizeof(T) + 2 > cap) {
                    Reserve<false>(len + sizeof(T) + 2);
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

        ~Data() {
            Clear(true);
        }

        // len 清 0, 可彻底释放 buf
        XX_FORCE_INLINE void Clear(bool const &freeBuf = false) {
            if (freeBuf && cap) {
                delete[](buf - reserveLen);
                buf = nullptr;
                cap = 0;
            }
            len = 0;
            offset = 0;
        }

        // 计算内存对齐的工具函数
        XX_FORCE_INLINE static size_t Calc2n(size_t const &n) noexcept {
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
        XX_FORCE_INLINE static size_t Round2n(size_t const &n) noexcept {
            auto rtv = size_t(1) << Calc2n(n);
            if (rtv == n) return n;
            else return rtv << 1;
        }
    };
}
