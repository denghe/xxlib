#include <emscripten/emscripten.h>
#include <cstdint>
#include <memory>
#include <utility>

// ӳ�䵽 js bytes array �Թ��������ý���
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
    // �� js �� g �����ڴ�ƫ�Ƶ�ַ
    void* EMSCRIPTEN_KEEPALIVE GetG() {
        return &g;
    }

    /*
    1. js �������� 10 �ֽڴ������ݵ� g.buf, len Ϊʵ�ʴ��ݳ���
    2. ���� 7bit �䳤 ���� uint64 д�� g.u
    3. �ɹ����� offset. ������ ���� �к�
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

    // ReadU64 �� �з��Ű�
	int EMSCRIPTEN_KEEPALIVE R64(int len) {
        int r = RU64(len);
        if (r > 0) {
            g.i = ZigZagDecode(g.u);
        }
        return r;
	}
    
    /*
        1. �� g.buf д��һ�� (double)uint64 ������ д�볤��
        2. js �� g.buf memcpy ������
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

    // WriteU64 �� �з��Ű�
    int EMSCRIPTEN_KEEPALIVE W64(double d) {
        auto du = ZigZagEncode(*(int64_t*)&d);
        return WU64(*(double*)&du);
    }

    /*
        1. �� g.buf д��һ�� (double)uint64 �� 10 ���� string ������ д�볤��
        2. js �� g.buf memcpy ������
    */
    int EMSCRIPTEN_KEEPALIVE TSU64(double d) {
        return ToString(*(uint64_t*)&d);
    }

    // ToStringDU64 �� �з��Ű�
    int EMSCRIPTEN_KEEPALIVE TS64(double d) {
        auto& n = *(int64_t*)&d;
        int idx = 0;
        if (n < 0) {
            n = -n;
            g.buf[idx++] = '-';
        }
        return ToString(n, idx);
    }

    // ���� double תΪ (double)uint64. �ᶪʧ����
    double EMSCRIPTEN_KEEPALIVE TU64(double d) {
        auto u = (uint64_t)d;
        return *(double*)&u;
    }

    // ���� double תΪ (double)int64. �ᶪʧ����
    double EMSCRIPTEN_KEEPALIVE T64(double d) {
        auto i = (int64_t)d;
        return *(double*)&i;
    }

    // todo: ������Ҫ�ṩ����� (double)int64 �����㺯��
}


/*

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
