#include <emscripten/emscripten.h>
#include <cstdint>
#include <memory>
#include <utility>

// 映射到 js bytes array 以供函数调用交互
union Global {
    uint8_t buf[32];
    int64_t i;
    uint64_t u;
};
inline static Global g;


inline int64_t ZigZagDecode(uint64_t in) {
    return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
}

inline uint64_t ZigZagEncode(int64_t in) {
    return (in << 1) ^ (in >> 63);
}

inline int ToString(uint64_t n, int idx = 0) {
    uint8_t buf[32];
    do {
        buf[idx++] = n % 10 + 48;
        n /= 10;
    } while (n);
    for (int i = 0; i < idx; ++i) g.buf[i] = buf[idx - i - 1];
    return idx;
}

extern "C" {
    // 供 js 拿 g 所在内存偏移地址
    void* EMSCRIPTEN_KEEPALIVE GetG() {
        return &g;
    }

    /*
    1. js 先填充最多 10 字节待读数据到 g.buf, len 为实际传递长度
    2. 函数 7bit 变长 读出 uint64 写入 g.u
    3. 成功返回 offset. 出错返回 负数 行号
    */
	int EMSCRIPTEN_KEEPALIVE RU64(int len) {
        int offset = 0;
        uint64_t u = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            if (offset == len) return -__LINE__;
            auto b = (uint64_t)g.buf[offset++];
            u |= uint64_t((b & 0x7Fu) << shift);
            if ((b & 0x80) == 0) {
                g.u = u;
                return offset;
            }
        }
        return -__LINE__;
    }

    // ReadU64 的 有符号版
	int EMSCRIPTEN_KEEPALIVE R64(int len) {
        int r = RU64(len);
        if (r > 0) {
            g.i = ZigZagDecode(g.u);
        }
        return r;
	}
    
    /*
        1. 向 g.buf 写入一个 (double)uint64 并返回 写入长度
        2. js 从 g.buf memcpy 走数据
    */
    int EMSCRIPTEN_KEEPALIVE WU64(double d) {
        auto& u = *(uint64_t*)&d;
        int len = 0;
        while (u >= 1 << 7) {
            g.buf[len++] = uint8_t((u & 0x7fu) | 0x80u);
            u = uint64_t(u >> 7);
        }
        g.buf[len++] = uint8_t(u);
        return len;
    }

    // WriteU64 的 有符号版
    int EMSCRIPTEN_KEEPALIVE W64(double d) {
        auto du = ZigZagEncode(*(int64_t*)&d);
        return WU64(*(double*)&du);
    }

    /*
        1. 向 g.buf 写入一个 (double)uint64 的 10 进制 string 并返回 写入长度
        2. js 从 g.buf memcpy 走数据
    */
    int EMSCRIPTEN_KEEPALIVE TSU64(double d) {
        return ToString(*(uint64_t*)&d);
    }

    // ToStringDU64 的 有符号版
    int EMSCRIPTEN_KEEPALIVE TS64(double d) {
        auto& n = *(int64_t*)&d;
        int idx = 0;
        if (n < 0) {
            n = -n;
            g.buf[idx++] = '-';
        }
        return ToString(n, idx);
    }

    // 正常 double 转为 (double)uint64. 会丢失精度
    double EMSCRIPTEN_KEEPALIVE TU64(double d) {
        auto u = (uint64_t)d;
        return *(double*)&u;
    }

    // 正常 double 转为 (double)int64. 会丢失精度
    double EMSCRIPTEN_KEEPALIVE T64(double d) {
        auto i = (int64_t)d;
        return *(double*)&i;
    }

    // todo: 可能需要提供更多的 (double)int64 的运算函数
}


/*

        // 尝试加载 webm 版本的 WriteU64 Write64 实现替换掉 Data 的成员函数( firefox 不替换快一丝 )
        if (navigator.userAgent.toLowerCase().indexOf('firefox') === -1) {
            try {
                let u8a = Uint8Array.from(window.atob(
                    'AGFzbQEAAAABIQdgAAF/YAF/AX5gAX4Bf2AAAGACf38Bf2ABfwBgAX8BfwMMCwMAAQECAgQABQYABAUBcAECAgUGAQGAAoACBgkBfwFBoIjAAgsHqQENBm1lbW9yeQIABkdldEJ1ZgABB1JlYWRVNjQAAgZSZWFkNjQAAwhXcml0ZVU2NAAEB1dyaXRlNjQABQZhZGRUd28ABhlfX2luZGlyZWN0X2Z1bmN0aW9uX3RhYmxlAQALX2luaXRpYWxpemUAABBfX2Vycm5vX2xvY2F0aW9uAAoJc3RhY2tTYXZlAAcMc3RhY2tSZXN0b3JlAAgKc3RhY2tBbGxvYwAJCQcBAEEBCwEACvwCCwMAAQsFAEGACAuPAQIDfgR/Qf8BIQQCQAJAIABBCiAAQQpIGyIARQ0AIABBAWsiAEEJIABBCUkbIQZBACEAA0ACQCAAQQFqIQUgAEGACGoxAAAiA0L/AIMgAoYgAYQhASADQoABg1ANACACQgd8IQIgACAGRiEHIAUhACAHRQ0BDAILCyAFIQQMAQtCACEBC0GPCCAEOgAAIAELNgIBfwF+IwBBEGsiASQAIAEgABACNwMIIAEpAwgiAkIBiEIAIAJCAYN9hSECIAFBEGokACACC0sBAn8gAEKAAVoEQANAIAFBgAhqIACnQYABcjoAACABQQFqIQEgAEL//wBWIQIgAEIHiCEAIAINAAsLIAFBgAhqIAA8AAAgAUEBagsxAQJ/IwBBEGsiASQAIAEgADcDCCABKQMIIgBCAYYgAEI/h4UQBCECIAFBEGokACACCwcAIAAgAWoLBAAjAAsGACAAJAALEAAjACAAa0FwcSIAJAAgAAsFAEGQCAs='
                ), (v) => v.charCodeAt(0));
                let wa = new WebAssembly.Instance(new WebAssembly.Module(u8a), {}).exports;

                Data.waUa = new Uint8Array(wa.memory.buffer, wa.GetBuf(), 16);
                Data.wa = wa;

                Data.prototype.Wvu64 = function (v) {
                    let len = this.Ensure(10);
                    let siz = Data.wa.WriteU64(v);
                    this.ua.set(Data.waUa.subarray(0, siz), len);
                    this.len = len + siz;
                };

                Data.prototype.Wvi64 = function (v) {
                    let len = this.Ensure(10);
                    let siz = Data.wa.Write64(v);
                    this.ua.set(Data.waUa.subarray(0, siz), len);
                    this.len = len + siz;
                };

                Data.prototype.Rvu64 = function () {
                    let left = Math.min(this.len - this.offset, 10);
                    Data.waUa.set(this.ua.subarray(this.offset, this.offset + left), 0);
                    let v = Data.wa.ReadU64(left);
                    let siz = Data.waUa[15];
                    if (siz == 255) throw new Error("Rvu64 out of range");
                    this.offset += siz;
                    return BigInt.asUintN(64, v);
                };

                Data.prototype.Rvi64 = function () {
                    let left = Math.min(this.len - this.offset, 10);
                    Data.waUa.set(this.ua.subarray(this.offset, this.offset + left), 0);
                    let v = Data.wa.Read64(left);
                    let siz = Data.waUa[15];
                    if (siz == 255) throw new Error("Rvi64 out of range");
                    this.offset += siz;
                    return v;
                };
            }
            catch (e) {
                console.log(e);
            }
        }


        // 测试 Wvi64 js 版 和 wasm 版的性能差异.
        // 测试结果：
        // chrome js 版 25xx 45xx ms, wasm 版 4xx 4xx ms
        // firefox js 版 & wasm 版 均为 8xx ms
        {
            let d = new Data();
            let t = new Date().getTime();
            for (let i = 0; i < 1000000; ++i) {
                d.len = 0;
                d.Wvi64(BigInt(-i));
                d.Wvi64(BigInt(i));
                d.Wvu64(BigInt.asUintN(64, BigInt(-i)));
                d.Wvu64(BigInt.asUintN(64, BigInt(i)));
            }
            console.log(new Date().getTime() - t);
            console.log(d.View());

            t = new Date().getTime();
            let c = 0n;
            for (let i = 0; i < 1000000; ++i) {
                d.offset = 0;
                c += d.Rvi64();
                c += d.Rvi64();
                c += d.Rvu64();
                c += d.Rvu64();
            }
            console.log(new Date().getTime() - t);
            console.log(c);
        }


*/
