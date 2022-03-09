#include <emscripten/emscripten.h>
#include <cstdint>
#include <utility>

// 供 js 转为 ByteArray + UInt8Array, 读 + 写 实现 传参 & 返回
// Write 后从该处复制结果, Read 前复制目标数据到此，Read 后 buf[15] 为 offset 增量
// 如果 buf[15] 为 255 则表示溢出
uint8_t buf[16];

inline int64_t ZigZagDecode(uint64_t const& in) {
    return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
}

inline uint64_t ZigZagEncode(int64_t const& in) {
    return (in << 1) ^ (in >> 63);
}

extern "C" {
    // 从 js 拿 buf 指针
    uint8_t* EMSCRIPTEN_KEEPALIVE GetBuf() {
        return buf;
    }

    // issue: 下面这个函数还有点问题

    // 从 buf 读出一个 uint64 并返回
	uint64_t EMSCRIPTEN_KEEPALIVE ReadU64(int leftLen) {
        int offset = 0;
        uint64_t u = 0;
        if (leftLen > 10) leftLen = 10;
        for (int shift = 0; shift < 64; shift += 7) {
            if (offset == leftLen) break;
            auto b = (uint64_t)buf[offset++];
            u |= uint64_t((b & 0x7Fu) << shift);
            if ((b & 0x80) == 0) {
                buf[15] = (uint8_t)offset;
                return u;
            }
        }
        buf[15] = 0xFFu;
        return 0;
    }

    // 从 buf 读出一个 int64 并返回
	int64_t EMSCRIPTEN_KEEPALIVE Read64(int leftLen) {
        return ZigZagDecode(ReadU64(leftLen));
	}

    // 向 buf 写入一个 uint64 并返回 写入长度
    int EMSCRIPTEN_KEEPALIVE WriteU64(uint64_t u) {
        int len = 0;
        while (u >= 1 << 7) {
            buf[len++] = uint8_t((u & 0x7fu) | 0x80u);
            u = uint64_t(u >> 7);
        }
        buf[len++] = uint8_t(u);
        return len;
    }

    // 向 buf 写入一个 int64 并返回 写入长度
    int EMSCRIPTEN_KEEPALIVE Write64(int64_t v) {
        return WriteU64(ZigZagEncode(v));
    }

    // test
    int EMSCRIPTEN_KEEPALIVE addTwo(int a, int b) {
        return a + b;
    }
}

/*

// 提速思路: Data 的 buf 可以直接来自 wasm 的 buf. 这样就能省掉一次 memcpy
// 甚至 len, offset 也可以内置到 buf 的开头直接交互


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
