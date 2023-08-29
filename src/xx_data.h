#pragma once
#include "xx_typetraits.h"
#include "xx_mem.h"

namespace xx {

    // 基础二进制数据跨度/引用容器( buf + len ) 类似 C++20 的 std::span
    // 注意: 因 追加 & 扩容 导致的数据失效问题, 追加自身存储的数据时要小心，需要先 reserve

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

        // 可轻松转为 std::string_view
        XX_INLINE operator std::string_view() const {
            return { (char*)buf, len };
        }
    };

    // mem moveable tag
    template<>
    struct IsPod<Span, void> : std::true_type {};

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

        XX_INLINE uint8_t* GetBuf() const {
            return buf;
        }

        XX_INLINE size_t GetLen() const {
            return len;
        }

        [[nodiscard]] bool HasLeft() const {
            return len > offset;
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

        // fill 剩余 buf to dr
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadLeftBuf(Data_r& dr) {
            if (offset == len) return __LINE__;
            dr.Reset(buf + offset, len - offset);
            offset = len;
            return 0;
        }

        // 跳过 siz 字节不读. 返回非 0 则失败( 长度不足 )
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadJump(size_t const &siz) {
            assert(siz);
            if (offset + siz > len) return __LINE__;
            offset += siz;
            return 0;
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
            if constexpr (std::endian::native == std::endian::big) {
                v = BSwap(v);
            }
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长小尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedAt(size_t const &idx, T &v) {
            if (idx + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
            if constexpr (std::endian::native == std::endian::big) {
                v = BSwap(v);
            }
            return 0;
        }

        // 读 定长大尾数字. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedBE(T& v) {
            if (offset + sizeof(T) > len) return __LINE__;
            memcpy(&v, buf + offset, sizeof(T));
            if constexpr (std::endian::native == std::endian::little) {
                v = BSwap(v);
            }
            offset += sizeof(T);
            return 0;
        }

        // 从指定下标 读 定长大尾数字. 不改变 offset. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedBEAt(size_t const& idx, T& v) {
            if (idx + sizeof(T) >= len) return __LINE__;
            memcpy(&v, buf + idx, sizeof(T));
            if constexpr (std::endian::native == std::endian::little) {
                v = BSwap(v);
            }
            return 0;
        }

        // 读 定长小尾数字 数组. 返回非 0 则读取失败
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadFixedArray(T* const& tar, size_t const& siz) {
            assert(tar);
            if (offset + sizeof(T) * siz > len) return __LINE__;
            if constexpr (std::endian::native == std::endian::big) {
                auto p = buf + offset;
                T v;
                for (size_t i = 0; i < siz; ++i) {
                    memcpy(&v, p + i * sizeof(T), sizeof(T));
                    tar[i] = BSwap(v);
                }
            } else {
                memcpy(tar, buf + offset, sizeof(T) * siz);
            }
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


        // 从 buf[offset] 处填充一个指定长度的 string_view. 返回非 0 则读取失败
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadSV(std::string_view& sv, size_t const& siz) {
            if (offset + siz >= len) return __LINE__;
            auto s = (char*)buf + offset;
            offset += siz;
            sv = { s, siz };
            return 0;
        }

        // 从 buf[offset] 处填充一个 \0 string_view. 不会超过 buf 总长度. 返回非 0 则读取失败( 只会发生于进函数时 buf 已读光 )
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadCStr(std::string_view& sv) {
            return ReadSV(sv, strlen((char*)buf + offset));
        }
        [[maybe_unused]] [[nodiscard]] XX_INLINE int ReadCStr(std::string& s) {
            std::string_view sv;
            if (int r = ReadCStr(sv)) return r;
            s = sv;
            return 0;
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

    // mem moveable tag
    template<>
    struct IsPod<Data_r, void> : std::true_type {};


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

        // buf cap resize to len
        void Shrink() {
            if (!len) {
                Clear(true);
            } else if (cap > len * 2) {
                auto newBuf = ((uint8_t*)malloc(bufHeaderReserveLen + len)) + bufHeaderReserveLen;
                memcpy(newBuf, buf, len);
                free(buf - bufHeaderReserveLen);
                buf = newBuf;
                cap = len;
            }
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

        // fill likely. make instance
        template<typename T = int32_t, typename = std::enable_if_t<std::is_convertible_v<T, uint8_t>>>
        static Data_rw From(std::initializer_list<T> const &bytes) {
            xx::Data_rw d;
            d.Fill(bytes);
            return d;
        }

        template<typename T = int32_t, typename = std::enable_if_t<std::is_convertible_v<T, std::string_view>>>
        static Data_rw From(T const &sv) {
            xx::Data_rw d;
            d.WriteBuf(sv);
            return d;
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

        // for fill read data
        Span GetFreeRange() const {
            return {buf + len, cap - len};
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

        // WriteBuf 支持一下 literal string[_view] 以方便使用
        template<bool needReserve = true>
        XX_INLINE void WriteBuf(std::string const& sv) {
            WriteBuf<needReserve>(sv.data(), sv.size());
        }
        template<bool needReserve = true>
        XX_INLINE void WriteBuf(std::string_view const& sv) {
            WriteBuf<needReserve>(sv.data(), sv.size());
        }
        template<bool needReserve = true, size_t n>
        XX_INLINE void WriteBuf(char const(&s)[n]) {
            WriteBuf<needReserve>(s, n - 1);
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
            if constexpr (std::endian::native == std::endian::big) {
                v = BSwap(v);
            }
            memcpy(buf + len, &v, sizeof(T));
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Little Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedAt(size_t const &idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
            if constexpr (std::endian::native == std::endian::big) {
                v = BSwap(v);
            }
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
            if constexpr (std::endian::native == std::endian::little) {
                v = BSwap(v);
            }
            memcpy(buf + len, &v, sizeof(T));
            len += sizeof(T);
        }

        // 在指定 idx 写入 float / double / integer ( 定长 Big Endian )
        template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T>>>
        [[maybe_unused]] XX_INLINE void WriteFixedBEAt(size_t const& idx, T v) {
            if (idx + sizeof(T) > len) {
                Resize(sizeof(T) + idx);
            }
            if constexpr (std::endian::native == std::endian::little) {
                v = BSwap(v);
            }
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
            if constexpr (std::endian::native == std::endian::big) {
                auto p = buf + len;
                T v;
                for (size_t i = 0; i < siz; ++i) {
                    v = BSwap(ptr[i]);
                    memcpy(p + i * sizeof(T), &v, sizeof(T));
                }
            } else {
                memcpy(buf + len, ptr, sizeof(T) * siz);
            }
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

    // mem moveable tag
    template<size_t bufHeaderReserveLen>
    struct IsPod<Data_rw<bufHeaderReserveLen>, void> : std::true_type {};


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


namespace xx {
    /**********************************************************************************************************************/
    // 为 Data.Read / Write 提供简单序列化功能( 1字节整数, float/double  memcpy,   别的整数 包括长度 变长. std::string/xx::Data 写 长度 + 内容
    // 这里只针对 “基础类型”

    // 适配 Data
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_base_of_v<Data, T>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteVarInteger<needReserve>(in.len);
            d.WriteBuf<needReserve>(in.buf, in.len);
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz = 0;
            if (int r = d.ReadVarInteger(siz)) return r;
            if (d.offset + siz > d.len) return __LINE__;
            out.Clear();
            out.WriteBuf(d.buf + d.offset, siz);
            d.offset += siz;
            return 0;
        }
    };

    // 适配 Span / Data_r ( for buf combine, does not write len )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_base_of_v<Span, T> && !std::is_base_of_v<Data, T>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteBuf<needReserve>(in.buf, in.len);
        }
    };


    // 适配 1 字节长度的 数值( 含 double ) 或 float/double( 这些类型直接 memcpy )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t< (std::is_arithmetic_v<T> && sizeof(T) == 1) || std::is_floating_point_v<T> >> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed<needReserve>(in);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadFixed(out);
        }
    };

    // 适配 2+ 字节整数( 变长读写 )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_integral_v<T> && sizeof(T) >= 2>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteVarInteger<needReserve>(in);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.ReadVarInteger(out);
        }
    };

    // 适配 enum( 根据原始数据类型调上面的适配 )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_enum_v<T>>> {
        typedef std::underlying_type_t<T> UT;
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.Write<needReserve>((UT const&)in);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.Read((UT&)out);
        }
    };

    // 适配 std::string_view ( 写入 变长长度 + 内容 )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string_view, std::decay_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteVarInteger<needReserve>(in.size());
            d.WriteBuf<needReserve>((char*)in.data(), in.size());
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz = 0;
            if (auto r = d.ReadVarInteger(siz)) return r;
            if (auto buf = d.ReadBuf(siz)) {
                out = std::string_view((char*)buf, siz);
                return 0;
            }
            return __LINE__;
        }
    };

    // 适配 literal char[len] string  ( 写入 变长长度-1 + 内容. 不写入末尾 0 )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<IsLiteral_v<T>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            DataFuncs<std::string_view, void>::Write<needReserve>(d, std::string_view(in));
        }
    };

    // 适配 std::string ( 写入 变长长度 + 内容 )
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<std::is_same_v<std::string, std::decay_t<T>>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            DataFuncs<std::string_view, void>::Write<needReserve>(d, std::string_view(in));
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz = 0;
            if (auto r = d.ReadVarInteger(siz)) return r;
            if (d.offset + siz > d.len) return __LINE__;
            out.assign((char*)d.buf + d.offset, siz);
            d.offset += siz;
            return 0;
        }
    };


    // 适配 std::optional<T>
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<IsOptional_v<T>/* && IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            if (in.has_value()) {
                d.Write<needReserve>((uint8_t)1, in.value());
            } else {
                d.Write<needReserve>((uint8_t)0);
            }
        }
        static inline int Read(Data_r& d, T& out) {
            char hasValue = 0;
            if (int r = d.Read(hasValue)) return r;
            if (!hasValue) return 0;
            if (!out.has_value()) {
                out.emplace();
            }
            return d.Read(out.value());
        }
    };

    // 适配 std::pair<K, V>
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<IsPair_v<T>/* && IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.Write<needReserve>(in.first, in.second);
        }
        static inline int Read(Data_r& d, T& out) {
            return d.Read(out.first, out.second);
        }
    };

    // 适配 std::tuple<......>
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<IsTuple_v<T>/* && IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            std::apply([&](auto const &... args) {
                d.Write<needReserve>(args...);
                }, in);
        }

        template<std::size_t I = 0, typename... Tp>
        static std::enable_if_t<I == sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
            return d.Read(std::get<I>(t));
        }

        template<std::size_t I = 0, typename... Tp>
        static std::enable_if_t < I < sizeof...(Tp) - 1, int> ReadTuple(Data_r& d, std::tuple<Tp...>& t) {
            if (int r = d.Read(std::get<I>(t))) return r;
            return ReadTuple<I + 1, Tp...>(d, t);
        }

        static inline int Read(Data_r& d, T& out) {
            return ReadTuple(d, out);
        }
    };


    // 适配 std::variant<......>
    template<typename T>
    struct DataFuncs<T, std::enable_if_t<IsVariant_v<T>>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            std::visit([&](auto&& v) {
                d.Write<needReserve>((size_t)in.index());
                d.Write<needReserve>(std::forward<decltype(v)>(v));
                }, in);
        }

        template<std::size_t I = 0, typename... Ts>
        static std::enable_if_t<I == sizeof...(Ts) - 1, int> ReadVariant(Data_r& d, std::variant<Ts...>& t, size_t const& index) {
            if (I == index) {
                t = std::tuple_element_t<I, std::tuple<Ts...>>();
                int r;
                std::visit([&](auto& v) {
                    r = d.Read(v);
                    }, t);
                return r;
            } else return -1;
        }
        template<std::size_t I = 0, typename... Ts>
        static std::enable_if_t < I < sizeof...(Ts) - 1, int> ReadVariant(Data_r& d, std::variant<Ts...>& t, size_t const& index) {
            if (I == index) {
                t = std::tuple_element_t<I, std::tuple<Ts...>>();
                int r;
                std::visit([&](auto& v) {
                    r = d.Read(v);
                    }, t);
                return r;
            } else return ReadVariant<I + 1, Ts...>(d, t, index);
        }

        static inline int Read(Data_r& d, T& out) {
            size_t index;
            if (int r = d.Read(index)) return r;
            return ReadVariant(d, out, index);
        }
    };


    // 适配 std::vector, std::array   // todo: queue / deque
    template<typename T>
    struct DataFuncs<T, std::enable_if_t< (IsVector_v<T> || IsArray_v<T>)/* && IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            if constexpr (IsVector_v<T>) {
                d.WriteVarInteger<needReserve>(in.size());
                if (in.empty()) return;
            }
            if constexpr (sizeof(T) == 1 || std::is_floating_point_v<T>) {
                d.WriteFixedArray<needReserve>(in.data(), in.size());
            } else if constexpr (std::is_integral_v<typename T::value_type>) {
                if constexpr (needReserve) {
                    auto cap = in.size() * (sizeof(T) + 2);
                    if (d.cap < cap) {
                        d.Reserve<false>(cap);
                    }
                }
                for (auto&& o : in) {
                    d.WriteVarInteger<false>(o);
                }
            } else {
                for (auto&& o : in) {
                    d.Write<needReserve>(o);
                }
            }
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz = 0;
            if constexpr (IsVector_v<T>) {
                if (int r = d.ReadVarInteger(siz)) return r;
                if (d.offset + siz > d.len) return __LINE__;
                out.resize(siz);
                if (siz == 0) return 0;
            } else {
                siz = out.size();
            }
            auto buf = out.data();
            if constexpr (sizeof(T) == 1 || std::is_floating_point_v<T>) {
                if (int r = d.ReadFixedArray(buf, siz)) return r;
            } else {
                for (size_t i = 0; i < siz; ++i) {
                    if (int r = d.Read(buf[i])) return r;
                }
            }
            return 0;
        }
    };

    // 适配 std::set, unordered_set
    template<typename T>
    struct DataFuncs<T, std::enable_if_t< IsSetSeries_v<T>/* && IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteVarInteger<needReserve>(in.size());
            if (in.empty()) return;
            if constexpr (std::is_integral_v<typename T::value_type>) {
                if constexpr (needReserve) {
                    auto cap = in.size() * (sizeof(T) + 2);
                    if (d.cap < cap) {
                        d.Reserve<false>(cap);
                    }
                }
                for (auto&& o : in) {
                    d.WriteVarInteger<false>(o);
                }
            } else {
                for (auto&& o : in) {
                    d.Write<needReserve>(o);
                }
            }
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz = 0;
            if (int r = d.Read(siz)) return r;
            if (d.offset + siz > d.len) return __LINE__;
            out.clear();
            if (siz == 0) return 0;
            for (size_t i = 0; i < siz; ++i) {
                if (int r = Read_(d, out.emplace())) return r;
            }
            return 0;
        }
    };

    // 适配 std::map unordered_map
    template<typename T>
    struct DataFuncs<T, std::enable_if_t< IsMapSeries_v<T> /*&& IsBaseDataType_v<T>*/>> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteVarInteger<needReserve>(in.size());
            for (auto&& kv : in) {
                d.Write<needReserve>(kv.first, kv.second);
            }
        }
        static inline int Read(Data_r& d, T& out) {
            size_t siz;
            if (int r = d.Read(siz)) return r;
            if (siz == 0) return 0;
            if (d.offset + siz * 2 > d.len) return __LINE__;
            for (size_t i = 0; i < siz; ++i) {
                MapSeries_Pair_t<T> kv;
                if (int r = d.Read(kv.first, kv.second)) return r;
                out.insert(std::move(kv));
            }
            return 0;
        }
    };

    // for read & write float ( value is uint16 )
    // example: d.Read( (xx::RWFloatUInt16&)x )
    struct RWFloatUInt16 {
        float v;
    };
    template<typename T>
    struct DataFuncs<T, std::enable_if_t< std::is_base_of_v<RWFloatUInt16, T> >> {
        template<bool needReserve = true>
        static inline void Write(Data& d, T const& in) {
            d.WriteFixed((uint16_t)in.v);
        }
        static inline int Read(Data_r& d, T& out) {
            uint16_t tmp;
            auto r = d.ReadFixed(tmp);
            out.v = tmp;
            return r;
        }
    };

}
