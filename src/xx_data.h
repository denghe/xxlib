#pragma once

#include "xx_bits.h"
#include "xx_macro.h"

#include <initializer_list>
#include <utility>
#include <cassert>
#include <cstring>      // memcmp

namespace xx {

    // 基础二进制数据跨度/引用容器( buf + len ) 类似 C++20 的 std::span
    struct Span {
        uint8_t* buf;
        size_t len;

        Span()
            : buf(nullptr), len(0) {
        }

        Span(Span const& o) = default;

        Span& operator=(Span const& o) = default;


        // 引用一段数据
        [[maybe_unused]] Span(void const* const& buf, size_t const& len)
            : buf((uint8_t*)buf), len(len) {
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        [[maybe_unused]] explicit Span(T const& d)
            : buf((uint8_t*)d.buf), len(d.len) {
        }

        // 引用一段数据
        [[maybe_unused]] XX_INLINE void Reset(void const* const& buf_, size_t const& len_) {
            buf = (uint8_t*)buf_;
            len = len_;
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        [[maybe_unused]] XX_INLINE void Reset(T const& d, size_t const& offset_ = 0) {
            Reset(d.buf, d.len, offset_);
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        Span& operator=(T const& o) {
            Reset(o.buf, o.len);
            return *this;
        }

        // 判断数据是否一致
        XX_INLINE bool operator==(Span const& o) const {
            if (&o == this) return true;
            if (len != o.len) return false;
            return 0 == memcmp(buf, o.buf, len);
        }

        XX_INLINE bool operator!=(Span const& o) const {
            return !this->operator==(o);
        }

        // 下标只读访问
        XX_INLINE uint8_t const& operator[](size_t const& idx) const {
            assert(idx < len);
            return buf[idx];
        }

        // 下标可写访问
        XX_INLINE uint8_t& operator[](size_t const& idx) {
            assert(idx < len);
            return buf[idx];
        }

        // 供 if 简单判断是否为空
        XX_INLINE operator bool() const {
            return len != 0;
        }
    };

    // Data 序列化 / 反序列化 基础适配模板
    template<typename T, typename ENABLED>
    struct DataFuncs;

    // 基础二进制数据跨度/引用容器 附带基础 流式读 功能( offset )
    struct Data_r : Span {
        size_t offset;

        Data_r()
                : offset(0) {
        }

        Data_r(Data_r const& o) = default;

        Data_r& operator=(Data_r const& o) = default;


        // 引用一段数据
        [[maybe_unused]] Data_r(void const *const &buf, size_t const &len, size_t const &offset = 0) {
            Reset(buf, len, offset);
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        [[maybe_unused]] Data_r(T const& d, size_t const& offset = 0) {
            Reset(d.buf, d.len, offset);
        }

        // 引用一段数据
        [[maybe_unused]] XX_INLINE void Reset(void const *const &buf_, size_t const &len_, size_t const &offset_ = 0) {
            this->Span::Reset(buf_, len_);
            offset = offset_;
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        [[maybe_unused]] XX_INLINE void Reset(T const& d, size_t const &offset_ = 0) {
            Reset(d.buf, d.len, offset_);
        }

        // 引用一个 含有 buf + len 成员的对象的数据
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        Data_r& operator=(T const& o) {
            Reset(o.buf, o.len);
            return *this;
        }

        // 判断数据是否一致
        XX_INLINE bool operator==(Data_r const &o) {
            return this->Span::operator==(o);
        }

        XX_INLINE bool operator!=(Data_r const &o) {
            return !this->operator==(o);
        }

        [[nodiscard]] size_t LeftLen() const {
            return len - offset;
        }

        [[nodiscard]] Span LeftSpan() const {
            return Span(buf + offset, len - offset);
        }

        [[nodiscard]] Data_r LeftData_r(size_t const& offset_ = 0) const {
            return Data_r(buf + offset, len - offset, offset_);
        }

        /***************************************************************************************************************************/

        // 返回剩余 buf ( 不改变 offset )
        [[maybe_unused]] [[nodiscard]] XX_INLINE std::pair<uint8_t*, size_t> GetLeftBuf() {
            return { buf + offset, len - offset };
        }

        // 读 定长buf 到 tar. 返回非 0 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadBuf(void *const &tar, size_t const &siz) {
            assert(tar);
            if (offset + siz > len) return __LINE__;
            memcpy(tar, buf + offset, siz);
            offset += siz;
            return 0;
        }

        // 从指定下标 读 定长buf. 不改变 offset. 返回非 0 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadBufAt(size_t const &idx, void *const &tar, size_t const &siz) const {
            assert(tar);
            if (idx + siz > len) return __LINE__;
            memcpy(tar, buf + idx, siz);
            return 0;
        }

        // 读 定长buf 起始指针 方便外面 copy. 返回 nullptr 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_INLINE void* ReadBuf(size_t const& siz) {
            if (offset + siz > len) return nullptr;
            auto bak = offset;
            offset += siz;
            return buf + bak;
        }

        // 从指定下标 读 定长buf 起始指针 方便外面 copy. 返回 nullptr 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_INLINE void* ReadBufAt(size_t const& idx, size_t const& siz) const {
            if (idx + siz > len) return nullptr;
            return buf + idx;
        }

        // 读 定长小尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixed(T &v) {
            if (offset + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + offset, sizeof(T));
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长小尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            return 0;
        }

        // 读 定长大尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedBE(T &v) {
            if (offset + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + offset, sizeof(T));
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长大尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedBEAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) >= len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            return 0;
        }

        // 读 定长小尾数字 数组. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedArray(T* const& tar, size_t const& siz) {
            assert(tar);
            if (offset + sizeof(T) * siz > len) return __LINE__;
#ifdef __BIG_ENDIAN__
            auto p = buf + offset;
            T v;
            for (size_t i = 0; i < siz; ++i) {
                memcpy(&v, p + i * sizeof(T), sizeof(T));
                tar[i] = BSwap(v);
            }
#else
            memcpy(tar, buf + offset, sizeof(T) * siz);
#endif
            offset += sizeof(T) * siz;
            return 0;
        }


        // 读 变长整数. 返回非 0 则读取失败
        template<typename T>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadVarInteger(T &v) {
            using UT = std::make_unsigned_t<T>;
            UT u(0);
            for (size_t shift = 0; shift < sizeof(T) * 8; shift += 7) {
                if (offset == len) return __LINE__;
                auto b = (UT) buf[offset++];
                u |= UT((b & 0x7Fu) << shift);
                if ((b & 0x80) == 0) {
                    if constexpr (std::is_signed_v<T>) {
                        if constexpr (sizeof(T) <= 4) v = ZigZagDecode(uint32_t(u));
                        else v = ZigZagDecode(uint64_t(u));
                    } else {
                        v = u;
                    }
                    return 0;
                }
            }
            return __LINE__;
        }

        // 读出并填充到变量. 可同时填充多个. 返回非 0 则读取失败
        template<typename ...TS>
        int Read(TS&...vs) {
            return ReadCore(vs...);
        }

    protected:
        template<typename T, typename ...TS>
        int ReadCore(T& v, TS&...vs);
        template<typename T>
        int ReadCore(T& v);
    };


    /***************************************************************************************************************************/
    /***************************************************************************************************************************/


    // 基础二进制数据容器 附带基础 流式读写 功能，可配置预留长度方便有些操作在 buf 最头上放东西
    template<size_t bufHeaderReserveLen = 0>
    struct Data_rw : Data_r {
        size_t cap;

        // buf = len = offset = cap = 0
        Data_rw()
            : cap(0) {
        }

        // 便于读取模板参数 buf 前面的内存预留长度
        inline static size_t constexpr GetBufHeaderReserveLen() {
            return bufHeaderReserveLen;
        }

        // unsafe: 直接设置成员数值, 常用于有把握的"借壳" 读写( 不会造成 Reserve 操作的 ), 最后记得 Reset 还原
        [[maybe_unused]] XX_INLINE void Reset(void const* const& buf_ = nullptr, size_t const& len_ = 0, size_t const& offset_ = 0, size_t const& cap_ = 0) {
            this->Data_r::Reset(buf_, len_, offset_);
            cap = cap_;
        }

        // 预分配空间
        [[maybe_unused]] explicit Data_rw(size_t const &cap)
                : cap(cap) {
            assert(cap);
            auto siz = Round2n(bufHeaderReserveLen + cap);
            buf = (new uint8_t[siz]) + bufHeaderReserveLen;
            this->cap = siz - bufHeaderReserveLen;
        }

        // 复制( offset = 0 )
        [[maybe_unused]] Data_rw(Span const& s)
            : cap(0) {
            WriteBuf(s.buf, s.len);
        }

        // 复制, 顺便设置 offset
        [[maybe_unused]] Data_rw(void const* const& ptr, size_t const& siz, size_t const& offset_ = 0)
            : cap(0) {
            WriteBuf(ptr, siz);
            offset = offset_;
        }

        // 复制( offset = 0 )
        Data_rw(Data_rw const &o)
            : cap(0) {
            operator=(o);
        }

        // 复制( offset = 0 )
        XX_INLINE Data_rw& operator=(Data_rw const& o) {
            return operator=<Data_rw>(o);
        }

        // 复制含有 buf + len 成员的类实例的数据( offset = 0 )
        template<typename T, typename = std::enable_if_t<std::is_class_v<T>>>
        XX_INLINE Data_rw& operator=(T const& o) {
            if (this == &o) return *this;
            Clear();
            WriteBuf(o.buf, o.len);
            return *this;
        }

        // 将 o 的数据挪过来
        Data_rw(Data_rw &&o) noexcept {
            memcpy((void*)this, &o, sizeof(Data_rw));
            memset((void*)&o, 0, sizeof(Data_rw));
        }

        // 交换数据
        XX_INLINE Data_rw &operator=(Data_rw &&o) noexcept {
            std::swap(buf, o.buf);
            std::swap(len, o.len);
            std::swap(cap, o.cap);
            std::swap(offset, o.offset);
            return *this;
        }

        // 判断数据是否一致( 忽略 offset, cap )
        XX_INLINE bool operator==(Data_rw const &o) const {
            return this->Span::operator==(o);
        }

        XX_INLINE bool operator!=(Data_rw const &o) const {
            return !this->operator==(o);
        }

        // 确保空间足够
        template<bool CheckCap = true>
        XX_NOINLINE void Reserve(size_t const &newCap) {
            if (CheckCap && newCap <= cap) return;

            auto siz = Round2n(bufHeaderReserveLen + newCap);
            //auto newBuf = (new uint8_t[siz]) + bufHeaderReserveLen;
            auto newBuf = ((uint8_t*)malloc(siz)) + bufHeaderReserveLen;
            if (len) {
                memcpy(newBuf, buf, len);
            }

            // 这里判断 cap 不判断 buf, 是因为 gcc 优化会导致 if 失效, 无论如何都会执行 free
            if (cap) {
                //delete[](buf - bufHeaderReserveLen);
                free(buf - bufHeaderReserveLen);
            }
            else {
                // let virtual memory -> physics
                for(size_t i = 0; i < newCap; i += 4096) (newBuf - bufHeaderReserveLen)[i] = 0;
            }
            buf = newBuf;
            cap = siz - bufHeaderReserveLen;
        }

        // 修改数据长度( 可能扩容 )。会返回旧长度
        XX_INLINE size_t Resize(size_t const &newLen) {
            if (newLen > cap) {
                Reserve<false>(newLen);
            }
            auto rtv = len;
            len = newLen;
            return rtv;
        }

        // 通过 初始化列表 填充内容. 填充前会先 Clear. 用法: d.Fill({ 1,2,3. ....})
        template<typename T = int32_t, typename = std::enable_if_t<std::is_convertible_v<T, uint8_t>>>
        [[maybe_unused]] XX_INLINE void Fill(std::initializer_list<T> const &bytes) {
            Clear();
            Reserve(bytes.size());
            for (auto &&b : bytes) {
                buf[len++] = (uint8_t) b;
            }
        }

        // 从头部移除指定长度数据( 常见于拆包处理移除掉已经访问过的包数据, 将残留部分移动到头部 )
        [[maybe_unused]] XX_INLINE void RemoveFront(size_t const &siz) {
            assert(siz <= len);
            if (!siz) return;
            len -= siz;
            if (len) {
                memmove(buf, buf + siz, len);
            }
        }


        /***************************************************************************************************************************/

        // 追加写入一段 buf( 不记录数据长度 )
        template<bool needReserve = true>
        XX_INLINE void WriteBuf(void const *const &ptr, size_t const &siz) {
            if constexpr (needReserve) {
                if (len + siz > cap) {
                    Reserve<false>(len + siz);
                }
            }
            memcpy(buf + len, ptr, siz);
            len += siz;
        }

        // WriteBuf 支持一下 string[_view] 以方便使用
        template<bool needReserve = true, typename SV>
        XX_INLINE void WriteBuf(SV const& sv) {
            WriteBuf<needReserve>(sv.data(), sv.size() * sizeof(decltype(sv.data()[0])));
        }

        // 在指定 idx 写入一段 buf( 不记录数据长度 )
        [[maybe_unused]] XX_INLINE void WriteBufAt(size_t const &idx, void const *const &ptr, size_t const &siz) {
            if (idx + siz > len) {
                Resize(idx + siz);
            }
            memcpy(buf + idx, ptr, siz);
        }


        // 追加写入 float / double / integer ( 定长 Little Endian )
        template<bool needReserve = true, typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixed(T v) {
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
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedAt(size_t const &idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __BIG_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + idx, &v, sizeof(T));
        }

        // 追加写入 float / double / integer ( 定长 Big Endian )
        template<bool needReserve = true, typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedBE(T v) {
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
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedBEAt(size_t const &idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
#ifdef __LITTLE_ENDIAN__
            v = BSwap(v);
#endif
            memcpy(buf + idx, &v, sizeof(T));
        }

        // 追加写入 float / double / integer ( 定长 Little Endian ) 数组
        template<bool needReserve = true, typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedArray(T const* const& ptr, size_t const& siz) {
            assert(ptr);
            if constexpr (needReserve) {
                if (len + sizeof(T) * siz > cap) {
                    Reserve<false>(len + sizeof(T) * siz);
                }
            }
#ifdef __BIG_ENDIAN__
            auto p = buf + len;
            T v;
            for (size_t i = 0; i < siz; ++i) {
                v = BSwap(ptr[i]);
                memcpy(p + i * sizeof(T), &v, sizeof(T));
            }
#else
            memcpy(buf + len, ptr, sizeof(T) * siz);
#endif
            len += sizeof(T) * siz;
        }

        // 追加写入整数( 7bit 变长格式 )
        template<bool needReserve = true, typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteVarInteger(T const &v) {
            using UT = std::make_unsigned_t<T>;
            UT u(v);
            if constexpr (std::is_signed_v<T>) {
                if constexpr (sizeof(T) <= 4) u = ZigZagEncode(int32_t(v));
                else u = ZigZagEncode(int64_t(v));
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
        [[maybe_unused]] XX_INLINE size_t WriteJump(size_t const &siz) {
            auto bak = len;
            if constexpr (needReserve) {
                if (len + siz > cap) {
                    Reserve<false>(len + siz);
                }
            }
            len += siz;
            return bak;
        }

        // 跳过指定长度字节数不写。返回起始 指针
        template<bool needReserve = true>
        [[maybe_unused]] XX_INLINE uint8_t* WriteSpace(size_t const& siz) {
            return buf + WriteJump<needReserve>(siz);
        }

        // 支持同时写入多个值
        template<bool needReserve = true, typename ...TS>
        void Write(TS const& ...vs);

        // TS is base of Span. write buf only, do not write length
        template<bool needReserve = true, typename ...TS>
        void WriteBufSpans(TS const& ...vs) {
            (WriteBuf(vs.buf, vs.len), ...);
        }


        /***************************************************************************************************************************/

        ~Data_rw() {
            Clear(true);
        }

        // len 清 0, 可彻底释放 buf
        XX_INLINE void Clear(bool const &freeBuf = false) {
            if (freeBuf && cap) {
                //delete[](buf - bufHeaderReserveLen);
                free(buf - bufHeaderReserveLen);
                buf = nullptr;
                cap = 0;
            }
            len = 0;
            offset = 0;
        }
    };

    using Data = Data_rw<sizeof(size_t)*2>;
    using DataView = Data_r;

    /************************************************************************************/
    // Data 序列化 / 反序列化 基础适配模板
    template<typename T, typename ENABLED = void>
    struct DataFuncs {
        // 整数变长写( 1字节除外 ), double 看情况, float 拷贝内存, 容器先变长写长度
        template<bool needReserve = true>
        static inline void Write(Data& dw, T const& in) {
            assert(false);
        }
        // 返回非 0 表示操作失败
        static inline int Read(Data_r& dr, T& out) {
            assert(false);
            return 0;
        }
    };

    template<typename T, typename ...TS>
    int Data_r::ReadCore(T& v, TS&...vs) {
        if (auto r = DataFuncs<T>::Read(*this, v)) return r;
        return ReadCore(vs...);
    }
    template<typename T>
    int Data_r::ReadCore(T& v) {
        return DataFuncs<T>::Read(*this, v);
    }

    template<size_t bufHeaderReserveLen>
    template<bool needReserve, typename ...TS>
    void Data_rw<bufHeaderReserveLen>::Write(TS const& ...vs) {
        (DataFuncs<TS>::template Write<needReserve>(*this, vs), ...);
    }
}
