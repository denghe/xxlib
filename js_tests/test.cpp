#include <emscripten/emscripten.h>
#include <cstdint>
#include <utility>

// �� js תΪ ByteArray + UInt8Array, �� + д ʵ�� ���� & ����
// Write ��Ӹô����ƽ��, Read ǰ����Ŀ�����ݵ��ˣ�Read �� buf[15] Ϊ offset ����
// ��� buf[15] Ϊ 255 ���ʾ���
uint8_t buf[16];

inline int64_t ZigZagDecode(uint64_t const& in) {
    return (int64_t)(in >> 1) ^ (-(int64_t)(in & 1));
}

inline uint64_t ZigZagEncode(int64_t const& in) {
    return (in << 1) ^ (in >> 63);
}

extern "C" {
    // �� js �� buf ָ��
    uint8_t* EMSCRIPTEN_KEEPALIVE GetBuf() {
        return buf;
    }

    // issue: ��������������е�����

    // �� buf ����һ�� uint64 ������
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

    // �� buf ����һ�� int64 ������
	int64_t EMSCRIPTEN_KEEPALIVE Read64(int leftLen) {
        return ZigZagDecode(ReadU64(leftLen));
	}

    // �� buf д��һ�� uint64 ������ д�볤��
    int EMSCRIPTEN_KEEPALIVE WriteU64(uint64_t u) {
        int len = 0;
        while (u >= 1 << 7) {
            buf[len++] = uint8_t((u & 0x7fu) | 0x80u);
            u = uint64_t(u >> 7);
        }
        buf[len++] = uint8_t(u);
        return len;
    }

    // �� buf д��һ�� int64 ������ д�볤��
    int EMSCRIPTEN_KEEPALIVE Write64(int64_t v) {
        return WriteU64(ZigZagEncode(v));
    }

    // test
    int EMSCRIPTEN_KEEPALIVE addTwo(int a, int b) {
        return a + b;
    }
}

/*

// ����˼·: Data �� buf ����ֱ������ wasm �� buf. ��������ʡ��һ�� memcpy
// ���� len, offset Ҳ�������õ� buf �Ŀ�ͷֱ�ӽ���


        // ���Լ��� webm �汾�� WriteU64 Write64 ʵ���滻�� Data �ĳ�Ա����( firefox ���滻��һ˿ )
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


        // ���� Wvi64 js �� �� wasm ������ܲ���.
        // ���Խ����
        // chrome js �� 25xx 45xx ms, wasm �� 4xx 4xx ms
        // firefox js �� & wasm �� ��Ϊ 8xx ms
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
