"use strict";

// 数据存储 & 序列化 & 反序列化器
class Data {
    buf = null; // ArrayBuffer
    len = 0;    // 已 write 数据长
    offset = 0; // 已 read 偏移量

    // buf 的操作视图
    dv = null; // new DataView(this.buf);
    ua = null; // new Uint8Array(this.buf);
    ia = null; // new Int8Array(this.buf);

    // string 转换器
    static td = new TextDecoder();
    static te = new TextEncoder();

    // 类实例创建函数集合
    static fs = [];

    // 通过 cap 或 TypeArray 复制构造
    constructor(a) {
        let cap, isTypedArray;
        switch (typeof a) {
            case "null":
            case "undefined":
                cap = 64;
                break;
            case "number":
                cap = a;
                break;
            case "object":
                if (a.byteLength) {
                    cap = a.byteLength;
                    isTypedArray = true;
                    break;
                }
            default:
                throw new Error("new Data( cap or TypedArray expected )");
        }
        this.buf = new ArrayBuffer(cap);
        this.dv = new DataView(this.buf);
        this.ua = new Uint8Array(this.buf);
        this.ia = new Int8Array(this.buf);
        if (isTypedArray) {
            this.ua.set(a);
            this.len = cap;
        }
    }

    // 获取剩余可读取长度
    GetLeft() {
        return this.len - this.offset;
    }

    // 获取 buf 的视图
    View() {
        return this.ua.slice(0, this.len);
    }

    // 确保剩余空间 足够容纳 siz 并返回当前 len
    Ensure(siz) {
        let cap = this.buf.byteLength;
        let len = this.len;
        let newCap = len + siz;
        if (newCap <= cap) return len;
        while (cap < newCap) {
            cap *= 2;
        }
        let buf = new ArrayBuffer(cap);
        this.buf = buf;
        this.dv = new DataView(buf);
        this.ia = new Int8Array(buf);
        let ua = new Uint8Array(buf);
        ua.set(this.ua.slice(0, len));
        this.ua = ua;
        return len;
    }

    // 复位写长度和读偏移量
    Clear() {
        this.len = 0;
        this.offset = 0;
    }

    // 跳过 siz 空间不写。返回起始 len
    Wj(siz) {
        let len = this.Ensure(siz);
        this.len = len + siz;
        return len;
    }

    // 写入 定长uint8
    Wu8(v) {
        let len = this.Ensure(1);
        this.ua[len] = v;
        this.len = len + 1;
    }

    // 写入 定长int8
    Wi8(v) {
        let len = this.Ensure(1);
        this.ia[len] = v;
        this.len = len + 1;
    }

    // 写入 定长 bool
    Wb(v) {
        let len = this.Ensure(1);
        this.ua[len] = v ? 1 : 0;
        this.len = len + 1;
    }

    // 写入 定长uint32( 小尾 ) 到指定位置( 不扩容: unsafe )
    Wu32at(pos, v) {
        this.dv.setUint32(pos, v, true);
    }

    // 写入 定长uint32( 小尾 )
    Wu32(pos, v) {
        let len = this.Ensure(4);
        this.dv.setUint32(len, v, true);
        this.len = len + 4;
    }

    // 写入 定长float( 小尾 )
    Wf(v) {
        let len = this.Ensure(4);
        this.dv.setFloat32(len, v, true);
        this.len = len + 4;
    }

    // 写入 定长double( 小尾 )
    Wd(v) {
        let len = this.Ensure(8);
        this.dv.setFloat64(len, v, true);
        this.len = len + 8;
    }

    // 写入 变长无符号整数( 32b )
    Wvu(v) {
        let len = this.Ensure(5);
        let ua = this.ua;
        v >>>= 0;
        while (v >= 0x80) {
            ua[len++] = (v & 0x7f) | 0x80;
            v >>>= 7;
        }
        ua[len++] = v;
        this.len = len;
    }

    // 写入 变长整数( 32b )
    Wvi(v) {
        this.Wvu(((v |= 0) << 1) ^ (v >> 31));
    }

    // 写入 变长无符号整数( 64b )  BigInt.asUintN(64, ?n);
    Wvu64(v) {
        let len = this.Ensure(10);
        let ua = this.ua;
        while (v >= 0x80n) {
            ua[len++] = Number((v & 0x7fn) | 0x80n);
            v >>= 7n;
        }
        ua[len++] = Number(v);
        this.len = len;
    }

    // 写入 变长整数( 64b )   ?n   BigInt(?)
    Wvi64(v) {
        if (v >= 0) {
            v = BigInt.asUintN(64, v) * 2n;
        }
        else {
            v = BigInt.asUintN(64, -v) * 2n - 1n;
        }
        this.Wvu64(v);
    }

    // 写入 utf8 string ( 变长siz + text )
    Wstr(s) {
        let a = Data.te.encode(s);
        let siz = a.byteLength;
        this.Wvu(siz);
        if (siz > 0) {
            let len = this.Ensure(siz);
            this.ua.set(a, len);
            this.len = len + siz;
        }
    }

    // 写入 data ( 变长len + buf )
    Wdata(d) {
        let siz = d.len;
        this.Wvu(siz);
        let len = this.Ensure(siz);
        this.ua.set(d.ua.slice(0, siz), len);
        this.len = len + siz;
    }

    // 读出 定长uint8
    Ru8() {
        let offset = this.offset;
        if (offset + 1 > this.len) throw new Error("Ru8 out of range");
        this.offset = offset + 1;
        return this.ua[offset];
    }

    // 读出 定长int8
    Ri8() {
        let offset = this.offset;
        if (offset + 1 > this.len) throw new Error("Ri8 out of range");
        this.offset = offset + 1;
        return this.ia[offset];
    }

    // 读出 定长bool
    Rb() {
        let offset = this.offset;
        if (offset + 1 > this.len) throw new Error("Rb out of range");
        this.offset = offset + 1;
        return this.ua[offset] == 1;
    }

    // 读出 定长uint32( 小尾 ) 从指定位置
    Ru32at(pos) {
        if (pos + 4 > this.len) throw new Error("Ru32at out of range");
        return this.dv.getUint32(pos, true);
    }

    // 读出 定长float( 小尾 )
    Ru32() {
        let offset = this.offset;
        if (offset + 4 > this.len) throw new Error("Ru32 out of range");
        this.offset = offset + 4;
        return this.dv.getUint32(offset, true);
    }

    // 读出 定长float( 小尾 )
    Rf() {
        let offset = this.offset;
        if (offset + 4 > this.len) throw new Error("Rf out of range");
        this.offset = offset + 4;
        return this.dv.getFloat32(offset, true);
    }

    // 读出 定长double( 小尾 )
    Rd() {
        let offset = this.offset;
        if (offset + 8 > this.len) throw new Error("Rd out of range");
        this.offset = offset + 8;
        return this.dv.getFloat64(offset, true);
    }

    // 读出 变长无符号整数
    Rvu() {
        let v = 0x0;
        let ua = this.ua;
        let len = this.len;
        let offset = this.offset;
        for (let shift = 0; shift < 64; shift += 7) {
            if (offset == len) throw new Error("Rvu out of range");
            let b = ua[offset++];
            v |= (b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
        }
        this.offset = offset;
        return v;
    }

    // 读出 变长整数
    Rvi() {
        let n = this.Rvu();
        return ((n >>> 1) ^ -(n & 1)) | 0;
    }

    // 读出 变长无符号整数 BigInt
    Rvu64() {
        let v = 0n;
        let ua = this.ua;
        let len = this.len;
        let offset = this.offset;
        for (let shift = 0n; shift < 64n; shift += 7n) {
            if (offset == len) throw new Error("Rvu64 out of range");
            let b = BigInt(ua[offset++]);
            v |= (b & 0x7Fn) << shift;
            if ((b & 0x80n) == 0n) break;
        }
        this.offset = offset;
        return v;
    }

    // 读出 变长整数 BigInt
    Rvi64() {
        let n = this.Rvu64();
        return BigInt.asIntN(64, (n >> 1n) ^ -BigInt.asIntN(64, n & 1n));
    }

    // 读出 string
    Rstr() {
        let siz = this.Rvu();
        let offset = this.offset;
        this.offset = offset + siz;
        return Data.td.decode(this.ua.slice(offset, offset + siz));
    }

    // 读出 data
    Rdata() {
        let siz = this.Rvu();
        let offset = this.offset;
        this.offset = offset + siz;
        return new Data(this.ua.slice(offset, offset + siz));
    }



    // 类注册( c 需要从 ObjBase 继承 )
    static Register(c) {
        if (Data.fs[c.typeId]) throw new Error(`duplicate register typeId = ${c.typeId}`);
        Data.fs[c.typeId] = c;
    }

    // 入口函数: 始向 d 写入一个 类实例
    WriteRoot(o) {
        let m = new Map();
        m.set(o, 1);
        this.m = m;
        this.n = 1;
        this.Wvu(o.GetTypeId());
        o.Write(this);
    }

    // 入口函数: 开始从 d 读出一个 类实例 并返回
    ReadRoot() {
        this.m = new Map();
        return this.ReadFirst();
    }

    // 内部函数: 向 d 写入一个 类实例. 格式: idx + typeId + content
    Write(o) {
        if (o === null) {
            this.Wu8(0);
        }
        else {
            if (!(o instanceof ObjBase)) throw new Error("Write error: o is not ObjBase's child");
            let m = this.m;
            let n = m.get(o);
            if (n === undefined) {
                n = this.n + 1;
                this.n = n;
                m.set(o, n);
                this.Wvu(n);
                this.Wvu(o.GetTypeId());
                o.Write(this);
            }
            else {
                this.Wvu(n);
            }
        }
    }

    // 内部函数: 从 d 读出一个 类实例 并返回( 针对第一个特化 )
    ReadFirst() {
        let m = this.m;
        let typeId = this.Rvu();
        if (typeId === 0) throw new Error("ReadFirst error: typeId === 0");
        let c = Data.fs[typeId];
        if (c === undefined) throw new Error(`ReadFirst error: c === undefined. typeId = ${typeId}`);
        let v = new c();
        m.set(1, v);
        v.Read(this);
        return v
    }

    // 内部函数: 从 d 读出一个 类实例 并返回
    Read() {
        let n = this.Rvu();
        if (n === 0) return null;
        let m = this.m;

        let len = m.size;
        let typeId = 0;
        if (n === len + 1) {
            typeId = this.Rvu();
            if (typeId === 0) throw new Error("Read error: typeId === 0");
            let c = Data.fs[typeId];
            if (c === undefined) throw new Error(`Read error: c === undefined. typeId = ${typeId}`);
            let v = new c();
            m.set(n, v);
            v.Read(this);
            return v;
        }
        else {
            if (n > len) throw new Error(`Read error: n > len. n = ${n}, len = ${len}`);
            return m.get(n);
        }
    }
}

// 尝试加载 webm 版本的 WriteU64 Write64 实现替换掉 Data 的成员函数( firefox 替换了没改善 )
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

        console.log("Data wasm loaded.");
    }
    catch (e) {
        console.log(e);
    }
}

// 基类
class ObjBase {
    // static typeId = ?;
    GetTypeId() {
        return this.constructor.typeId;
    }
}
