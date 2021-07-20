const std = @import("std");
const root = @import("root");
const native_endian = @import("builtin").target.cpu.arch.endian();
const print = std.debug.print;
const assert = std.debug.assert;
const Allo = std.mem.Allocator;

pub fn main() void {
    print("{}", .{ maxSize(.{ A, B }) });
    var t:[maxSize(.{ A, B })] u8 align(8) = undefined;
}

// 利用 extern 布局 struct 指针硬转 模拟下继承

const A = extern struct {
    typeId:u32,
    pub fn a(this:*@This()) void {
        print("A\n", .{});
    }
};

const B = extern struct {
    base:A,
    pub fn a(this:*@This()) void {
        this.base.a();
    }
    pub fn b(this:*@This()) void {
        print("B\n", .{});
    }
};

pub fn maxSize(comptime args:anytype) usize {
    comptime {
        var r:usize = 0;
        for (args) |a| {
            if (r < @sizeOf(a)) {
                r = @sizeOf(a);
            }
        }
        return r;
    }
}


// zig build-exe .\src\main.zig -O ReleaseSmall --single-threaded --strip --library c -I src
// zig build-exe .\src\main.zig -O ReleaseSmall --single-threaded --strip --library c -I src -target x86_64-linux -dynamic

// const C = @cImport({
//     @cInclude("a.h");
// });

// pub fn main() void {
//     _=C.printf("hi %s %g\n", "asdf", @as(f32, 1.23));
//     var ptr = C.malloc(1024);
//     print("{s}\n", .{ @typeName(@TypeOf(ptr)) });
// }

// pub fn main() void {
//     var a = CreateAB(true){};
//     var b = CreateAB(false){};
//     PrintAB(a);
//     PrintAB(b);
// }

// const A = struct {
//     a:i32 = 1,
// };

// const B = struct {
//     b:i32 = 2,
// };

// pub fn CreateAB(comptime b:bool) type {
//     return if (b) A else B;
// }

// pub fn PrintAB(v:anytype) void {
//     const T = @TypeOf(v);
//     Dump(T);
//     if (@hasField(T, "a")) {
//         print("{}\n", .{ v.a });
//     }
//     if (@hasField(T, "b")) {
//         print("{}\n", .{ v.b });
//     }
// }

// pub fn Dump(comptime T:type) void {
//     const fs = std.meta.fields(T);
//     inline for(fs) |f| {
//         print("{}\n", .{ f });
//     }
//     const ds = std.meta.declarations(T);
//     inline for(ds) |d| {
//         print("{}\n", .{ d });
//     }
// }






// const Foo = struct {
//     const This = @This();
//     a:i32 = undefined,
//     b:i32 = undefined,
//     c:*Bar = undefined,
//     pub fn inita( this:*This, allo:*Allo ) void {
//         this.a = 0;
//         this.b = 1;
//         this.c = RAII(@TypeOf(this.c.*)).new(allo);
//     }
//     pub fn deinita( this:*This, allo:*Allo ) void {
//         RAII(@TypeOf(this.c.*)).delete(allo, this.c);
//     }
// };
// const Bar = struct {
//     const This = @This();
//     a:i32 = undefined,
//     b:i32 = undefined,
//     pub fn init( this:*This ) void {
//         this.a = 2;
//         this.b = 3;
//     }
// };

// pub fn main() void {
//     // var buffer:[1024*1024*4]u8 = undefined;
//     // var fba = std.heap.FixedBufferAllocator.init(&buffer);
//     // defer print("fba.end_index = {}\n", .{ fba.end_index });
//     // const a = &fba.allocator;

//     var gpa = std.heap.GeneralPurposeAllocator(.{}){}; // .{ .enable_memory_limit = true }){};
//     defer _ = gpa.deinit();
//     //gpa.setRequestedMemoryLimit(512);
//     const a = &gpa.allocator;

//     var foo = RAII(Foo).new(a);
//     defer RAII(Foo).delete(a, foo);
//     print("{}\n", .{ foo });
    
//     //print("fba.end_index = {}\n", .{ fba.end_index });
//     pressEnterContinue();
// }

// pub fn pressEnterContinue() void {
//     print("press Enter to continue...", .{});
//     var buf:[1]u8 = undefined;
//     _ = std.io.getStdIn().read(&buf) catch unreachable;
// }

// struct 接口规范:
// pub fn init( this:*T ) void
// pub fn inita( this:*T, allo:Allo ) void
// pub fn deinit( this:*T ) void

// for pub usingnamespace
pub fn TypeInfo(comptime T:type) type {
    return struct {
        pub const tIsStruct = @typeInfo(T) == .Struct;
        pub const tIsEnum = @typeInfo(T) == .Enum;
        pub const tIsUnion = @typeInfo(T) == .Union;
        pub const tHasFUnc = tIsStruct or tIsEnum or tIsUnion;
        pub const tHasInit = tHasFUnc and @hasDecl(T, "init");
        pub const tHasInita = tHasFUnc and @hasDecl(T, "inita");
        pub const tHasDeinita = tHasFUnc and @hasDecl(T, "deinita");
        pub const tHasDeinit = tHasFUnc and @hasDecl(T, "deinit");
    };
}

// 模拟 c++ raii
// 基本语法依赖:[new+]构造, defer 析构[+delete]
// 构造有2种，new:分配堆内存并初始化，placement new:在已有内存上初始化
// new 常见于 存放于 智能指针 之中的数据类型
// placement new 常见于 容器内 已批量分配的内存区域的 对象直接填值
// struct 函数需提供 构造 和 析构 以配合 new + delete
pub fn RAII(comptime T:type) type {
    return struct {
        pub usingnamespace TypeInfo(T);

        // auto call inita or init
        pub fn init(t:*T, allo:*Allo) void {
            if (tHasInita) {
                t.inita(allo);
            } else if (tHasInit) {
                t.init();
            }
        }

        // auto call deinit
        pub fn deinit(t:*T, allo:*Allo) void {
            if (tHasDeinita) {
                t.deinita(allo);
            } else if (tHasDeinit) {
                t.deinit();
            }
        }

        // malloc + auto call inita & init
        pub fn new(allo:*Allo) *T {
            var t = @ptrCast(*T, allo.alignedAlloc(u8, 8, @sizeOf(T)) catch unreachable);
            init(t, allo);
            return t;
        }

        // auto call deinit + free
        pub fn delete(allo:*Allo, t:*T) void {
            deinit(t, allo);
            allo.free( @ptrCast(*[@sizeOf(T)]u8, t) );
        }
    };
}

// 上述设计已知问题：不便托管给容器. 容器在移除对象时，不确定要如何释放内存
// 下面代码现有方案为：调用 T.init/a 来得到结构完整数据，并覆盖到容器内存
// 析构时只需要 deinit. 这意味着对象内存管理完全自己搞定
// 非要纠结的话，只需要补充个 T.pinit/a 系列，p == placement, 根据传入地址直接初始化，避免一次 copy
// 总结：可以加一组函数：create + release 来明确该类型只适合 new 成指针用，并且自己释放内存
// 而 init deinit 系列只针对已存在内存块操作，不释放该块，可能被 create release 间接调用


// 模拟 c++ std::shared_ptr
pub fn Shared(comptime T:type) type {
    return struct {
        const This = @This();
        pub usingnamespace TypeInfo(T);

        // memory layout:[16 bytes header][ T data ]
        // header = sharedCount + weakCount + typeId + flags
        // ptr = &[ T data ]
        ptr:?*T,
        allo:*std.mem.Allocator,

        pub fn sharedCount(this:This) u32 {
            return if (this.ptr) |p| @intToPtr(*u32, @ptrToInt(p) - 16).* else 0;
        }
        pub fn weakCount(this:This) u32 {
            return if (this.ptr) |p| @intToPtr(*u32, @ptrToInt(p) - 12).* else 0;
        }
        pub fn typeId(this:This) u32 {
            return if (this.ptr) |p| @intToPtr(*u32, @ptrToInt(p) - 8).* else 0;
        }
        pub fn flags(this:This) u32 {
            return if (this.ptr) |p| @intToPtr(*u32, @ptrToInt(p) - 4).* else 0;
        }
        pub fn empty(this:This) bool {
            return !this.ptr;
        }

        pub fn inita(allo:*std.mem.Allocator) This {
            return .{
                .ptr = null,
                .allo = allo,
            };
        }
        
        pub fn emplace(this:*This, v:T) void {
            this.deinit();
            var p = @ptrCast([*]u32, this.allo.alignedAlloc(u8, 8, @sizeOf(T) + 16) catch @panic(std.fmt.comptimePrint("{s}:{}:{} {s}", .{ @src().file, @src().line, 1, @src().fn_name })));
            p[0] = 1;
            p[1] = 0;
            p[2] = 0;
            p[3] = 0;
            this.ptr = @intToPtr(*T, @ptrToInt(p) + 16);
            this.ptr.?.* = v;
        }

        pub fn value(this:This) *T {
            return if (this.ptr) |p| p else @panic(std.fmt.comptimePrint("{s}:{}:{} {s}", .{ @src().file, @src().line, 1, @src().fn_name }));
        }
        pub fn setValue(this:This, v:T) void {
            if (this.ptr) |p| p.* = v else @panic(std.fmt.comptimePrint("{s}:{}:{} {s}", .{ @src().file, @src().line, 1, @src().fn_name }));
        }

        pub fn deinit(this:*This) void {
            if (this.ptr) |p| {
                var h = @intToPtr([*]u32, @ptrToInt(p) - 16);
                if (h[0] == 1) {
                     if (tHasDeinit) {
                         this.ptr.?.deinit();
                     }
                     this.ptr = null;
                     if (h[1] == 0) {
                         this.allo.free( @ptrCast(*[@sizeOf(T) + 16]u8, h) );
                     } else {
                         h[0] = 0;
                     }
                } else {
                     h[0] -= 1;
                     this.ptr = null;
                }
            }
        }

        pub fn clone(this:This) This {
            if (this.ptr) |p| {
                var h = @intToPtr([*]u32, @ptrToInt(p) - 16);
                h[0] += 1;
            }
            return .{
                .ptr = this.ptr,
                .allo = this.allo,
            };
        }

        pub fn cloneTo(this:This, o:*This) void {
            o.deinit();
            o.* = this.clone();
        }

        pub fn assign(this:*This, o:This) void {
            this.deinit();
            this.* = o.clone();
        }
    };
}

// 上述结构问题在于：多包了一层，用起来不是很方便，zig 没有 ->
// 更直接的办法似乎是类似下面的 Array，提供给别的 struct 继承
// 基础结构包含上述代码的 header 部分，派生类传入一个扩展结构
// 直接当作派生类指针用, 但被附加成员函数：release  clone. 容器做相应函数存在检测并调用
// 派生类需提供 allo 容器
// 思考：weak 包一层？类似上述 Shared?

pub fn SharedBase(comptime T:type) type {
    return struct {
        const This = @This();
        pub usingnamespace TypeInfo(T);
        pub usingnamespace T;
        sharedCount:u32 = undefined,
        weakCount:u32 = undefined,
        t:T = undefined,
        pub fn create(allo:*Allo) *T {
            var p = @ptrCast(*This, allo.alignedAlloc(u8, 8, @sizeOf(This)) catch unreachable);
            p.sharedCount = 1;
            t.allo = allo;
            if (tHasInita) {
                p.t.inita(allo);
            } else if (tHasInit) {
                p.t.init();
            }
            return &p.t;
        }
        pub fn release(this:*T) void {
            var p = @intToPtr(*This, @ptrToInt(p) - @byteOffsetOf(This, "t"));
            if (p.sharedCount == 1) {
                if (tHasDeinit) {
                    p.t.deinit();
                }
                if (p.weakCount == 0) {
                    allo.free( @ptrCast(*[@sizeOf(This)]u8, t) );
                } else {
                    p.sharedCount = 0;
                }
            } else {
                p.sharedCount -= 1;
            }
        }
        pub fn clone(this:*T) void {
            // todo
        }
    };
}




// 自动构造 & 析构的类似 c++ std::vector
pub fn Array(comptime T:type) type {
    return struct {
        const This = @This();
        pub usingnamespace TypeInfo(T);

        buf:?[]T,
        len:usize,
        allo:*std.mem.Allocator,
        pub fn cap(this:This) usize {
            return if (this.buf) |b| b.len else 0;
        }
        
        pub fn inita(allo:*std.mem.Allocator) This {
            return .{
                .buf = null,
                .len = 0,
                .allo = allo,
            };
        }

        pub fn deinit(this:*This) void {
            if (this.buf) |b| {
                this.clear();
                this.allo.free(b);
            }
        }

        pub fn clear(this:*This) void {
            if (this.buf) |b| {
                if (tHasDeinit) {
                    var i:usize = 0;
                    while (i < this.len) :(i += 1) {
                        this.buf.?[i].deinit();
                    }
                }
                this.len = 0;
            }
        }

        pub fn reserve(this:*This, newCap:usize) void {
            @setCold(true);
            if (this.buf) |b| {
                var c = b.len;
                if (c >= newCap) return;
                while (c < newCap) :(c *= 2) {}
                this.buf = this.allo.realloc(b, c) catch unreachable;
            } else {
                this.buf = this.allo.alloc(T, newCap) catch unreachable;
                // preload from virtual memory
                var i :usize = 0;
                while (i < newCap * @sizeOf(T)) :(i += 4096) {
                    @ptrCast([*]u8, &this.buf.?[0])[i] = 0;
                }
            }
        }

        pub fn at(this:This, idx:usize) *T {
            return &this.buf.?[idx];
        }

        pub fn alloc(this:*This) *T {
            const newCap = this.len + 1;
            if (this.cap() < newCap) {
                this.reserve(newCap);
            }
            const r = &this.buf.?[this.len];
            this.len += 1;
            return r;
        }

        // todo:erase, pop ...

        // for pub usingnamespace. C.base is This
        pub fn Ext(comptime C:type) type {
            return struct {
                pub fn cap(this:C) usize { return this.base.cap(); }
                pub fn len(this:C) usize { return this.base.len; }
                pub fn reserve(this:*C, newCap:usize) void { this.base.reserve(newCap); }
                pub fn inita(allo:*std.mem.Allocator) C { return .{ .base = This.inita(allo) }; }
                pub fn deinit(this:*C) void { this.base.deinit(); }
                pub fn clear(this:*C) void { this.base.clear(); }
                pub fn at(this:*C, idx:usize) *T { return this.base.at(idx); }
                pub fn alloc(this:*C) *T { return this.base.alloc(); }
            };
        }
    };
}

pub fn List(comptime T:type) type {
    return struct {
        const This = @This();
        const Base = Array(T);
        base:Base,
        pub usingnamespace Base.Ext(This);

        pub fn emplace(this:*This) *T {
            const r = this.alloc();
            if (Base.tHasInita) {
                r.* = T.inita(a.allo);
            }
            else if (Base.tHasInit) {
                r.* = T.init();
            } // ...
            return r;
        }

        pub fn push(this:*This, v:T) void {
            this.alloc().* = v;
        }
    };
}

pub const Data = struct {
    const T = u8;
    const This = @This();
    const Base = Array(T);
    base:Base,
    pub usingnamespace Base.Ext(This);

    pub fn writeFixed(this:*This, v:anytype) void {
        const t = @TypeOf(v);
        switch (@typeInfo(t)) {
             .Int, .Float => {
                if (native_endian == .Big) {
                    v = @byteSwap(t, v);
                }
            },
            else => {
                @compileError("writeFixed unsupported type:" ++ @typeName(@TypeOf(v)));
            },
        }
        const a = &this.base;
        const newCap = a.len + @sizeOf(t);
        if (a.cap() < newCap) {
            a.reserve(newCap);
        }
        @memcpy(@ptrCast([*]u8, &a.buf.?[a.len]), @ptrCast([*] const u8, &v), @sizeOf(t));
        a.len += @sizeOf(t);
    }
};

pub const String = struct {
    const T = u8;
    const This = @This();
    const Base = Array(u8);
    base:Base,
    pub usingnamespace Base.Ext(This);

    pub fn append(this:*This, v:anytype) void {
        // const a = &this.base;
        // var siz = std.fmt.count(comptime fmt:[]const u8, args:anytype)
        // const newCap = a.len + siz;
        // if (a.cap() < newCap) {
        //     a.reserve(newCap);
        // }
        // var buf = @ptrCast([*]u8, .....
        switch (@typeInfo(T)) {
            .Array => |info| {
                //std.fmt.bufPrint(buf:[]u8, "{s}", v);
            },
            else => {
                //std.fmt.bufPrint(buf:[]u8, "{}", v);
            }
        }
        // @memcpy(@ptrCast([*]u8, &a.buf.?[a.len]), @ptrCast([*] const u8, &v), @sizeOf(t));
        // a.len += @sizeOf(t);
    }
};





    // {
    //     var d = Data.inita(a);
    //     defer d.deinit();
    //     d.reserve(4000000000);
    //     var timer = std.time.Timer.start() catch unreachable;
    //     const t = timer.lap();
    //     var i:i32 = 0;
    //     while (i < 1000000000) :(i += 1) {
    //         d.writeFixed(@as(u32, 123));
    //     }
    //     print("elapsed_s = {}\n", .{ @intToFloat(f64, timer.read() - t) / std.time.ns_per_s });
    //     print("d.len = {}, d.cap = {}\n", .{ d.len(), d.cap() });
    //     //print("d.buf = {any}\n", .{ d.buf });
    // }
    




// pub const Data = struct {
//     const This = @This();
//     buf:?[]u8,
//     len:usize,
//     a:*std.mem.Allocator,
//     pub fn cap(this:This) usize {
//         return if (this.buf) |b| b.len else 0;
//     }
    
//     pub fn inita(a:*std.mem.Allocator) This {
//         return .{
//             .buf = null,
//             .len = 0,
//             .a = a,
//         };
//     }

//     pub fn deinit(this:*This) void {
//         if (this.buf) |b| {
//             this.a.free(b);
//         }
//     }

//     pub fn round2n(n:usize) usize {
//         var rtv = @as(usize, 1) << @intCast(u6, @bitSizeOf(usize) - 1 - @clz(usize, n));
//         return if (rtv == n) n else rtv << 1;
//     }
//     pub fn reserve(this:*This, newCap:usize) void {
//         @setCold(true);
//         if (this.buf) |b| {
//             if (b.len < newCap) {
//             this.buf = this.a.realloc(b, round2n(newCap)) catch unreachable;
//             }
//         } else {
//             this.buf = this.a.alloc(u8, round2n(newCap)) catch unreachable;
//             var i :usize = 0;
//             while (i < newCap) :(i += 4096) {
//                 this.buf.?[i] = 0;
//             }
//         }
//     }
    
//     pub fn writeFixed(this:*This, v:anytype) void {
//         const t = @TypeOf(v);
//         switch (@typeInfo(t)) {
//              .Int, .Float => {
//                 if (native_endian == .Big) {
//                     v = @byteSwap(t, v);
//                 }
//             },
//             else => {
//                 @compileError("writeFixed unsupported type:" ++ @typeName(@TypeOf(v)));
//             },
//         }

//         const newCap = this.len + @sizeOf(t);
//         if (this.cap() < newCap) {
//             this.reserve(newCap);
//         }
//         @memcpy(@ptrCast([*]u8, &this.buf.?[this.len]), @ptrCast([*] const u8, &v), @sizeOf(t));
//         this.len += @sizeOf(t);
//     }
// };





    // var arg_it = std.process.args();

    // const exe = arg_it.next(a) orelse unreachable catch unreachable;
    // defer a.free(exe);

    // assert(@TypeOf(exe) == [:0]u8);
    // print("exe = {s}\n", .{ exe });





    // pub fn round2n(n:usize) usize {
    //     var rtv = @as(usize, 1) << @intCast(u6, @bitSizeOf(usize) - 1 - @clz(usize, n));
    //     return if (rtv == n) n else rtv << 1;
    // }

    // pub const Error = error{
    //     OutOfMemory,
    //     InvalidRange,
    // };
    // pub fn reserve(this:*This, newCap:usize) Error!void {
    //     if (this.buf) |b| {
    //         if (b.len < newCap) {
    //             this.buf = this.a.realloc(b, round2n(newCap)) catch |e| {
    //                 return Error.OutOfMemory;
    //             };
    //         }
    //     } else {
    //         this.buf = this.a.alloc(u8, round2n(newCap)) catch |e| {
    //             return Error.OutOfMemory;
    //         };
    //     }
    // }
    //pub fn writeFixed(this:*This, v:anytype) Error!void {
    //     const t = @TypeOf(v);
    //     const ti = @typeInfo(t);
    //     switch (ti) {
    //          .Int, .Float => {
    //             if (native_endian == .Big) {
    //                 v = @byteSwap(t, v);
    //             }
    //         },
    //         else => {
    //             @compileError("writeFixed unsupported type:" ++ @typeName(@TypeOf(v)));
    //         },
    //     }
    //     try this.reserve(this.len + @sizeOf(t));
    //     @memcpy(@ptrCast([*]u8, &this.buf.?[this.len]), @ptrCast([*] const u8, &v), @sizeOf(t));
    //     this.len += @sizeOf(t);
    // }




















//const fs = std.fs;


    //var buffer:[1024*1024]u8 = undefined;
    //var fba = std.heap.FixedBufferAllocator.init(&buffer);
    //print("fba.end_index = {}\n", .{ fba.end_index });
    //const a = &fba.allocator;

    // var buf :[ fs.MAX_PATH_BYTES:0 ] u8 = undefined;
    // var exedir = fs.selfExeDirPath(&buf) catch unreachable;
    // print("exedir = {s}\n", .{ exedir });

    //print("fba.end_index = {}\n", .{ fba.end_index });

    //defer{pressEnterContinue();}
    //print("fs.MAX_PATH_BYTES = {}\n", .{ fs.MAX_PATH_BYTES });


// const std = @import("std");
// const print = std.debug.print;

// pub fn isUnsigned(comptime T:type) bool {  // callconv(.Inline)
//     for(@typeName(T)) |c, i| {
//         if (c == 'u' or c == 'U') 
//             return true;
//     }
//     return false;
// }

// const myuint256 = struct {
//     a :[]u8
// };

// pub fn main() void {
//     print("{} {} {}", .{ comptime isUnsigned(i32), comptime isUnsigned(u32), comptime isUnsigned(myuint256) });
//     var x :u32 = std.math.maxInt(u32);
//     x = if (std.math.mul(u32, x, 2)) |result| result else |err| {
//         print("{s}", .{ std.fmt.comptimePrint("{s}!", .{ "Overflow" }) });
//         return;
//     };
//     if (std.math.mul(u32, x, 2)) |result| {
//         x = result;
//     } else |err| {
//         print("{s}", .{ std.fmt.comptimePrint("{s}!", .{ "Overflow" }) });
//         x = 0;
//     }

//     // var s3 = S3 {
//     //     .a = 1,
//     //     .b = 2
//     // };
// }

// const S1 = struct {
//     a:i32
// };

// const S2 = struct {
//     b:i64
// };

// const S3 = S1 || S2;











// pub fn xxxx(s:*Shared(i32)) void {
//     defer s.deinit();
//     print("xxxx {}\n", .{ s.sharedCount() });
// }






// pub const xxxxxxxx = .xxxxxxxxxxxxxxxxxx;
// pub const abcdefg = List(i32).inita(std.heap.page_allocator);

// pub fn xxxxxx() void {
//     print("this is xxxxxx\n", .{});
// }

// pub fn main() void {
//     print("{}\n", .{ @TypeOf(root) });
    // if (@hasDecl(root, "xxxxxxxx")) {
    //     print("{}\n", .{ root.xxxxxxxx });
    // }
    // if (@hasDecl(root, "abcdefg")) {
    //     print("{}\n", .{ root.abcdefg });
    // }
    // if (@hasDecl(root, "xxxxxx")) {
    //     xxxxxx();
    // }
    //return;



    // var ab = AB{ .a = A{ .base = C{ .typeId = 1, .counter = 0 }} };
    // _=ab;

    // var gpa = std.heap.GeneralPurposeAllocator(.{}){}; // .{ .enable_memory_limit = true }){};
    // defer _ = gpa.deinit();
    // //gpa.setRequestedMemoryLimit(512);
    // const a = &gpa.allocator;

    // {
    //     var list = List(AB).inita(a);
    //     defer list.deinit();
    //     var i:usize = 0;
    //     var k:usize = 0;
    //     while (i<100000):(i+=1) {
    //         list.emplace().* = AB{ .a = A{ .base = C{ .typeId = A.typeId, .counter = 0 }} };
    //         list.emplace().* = AB{ .b = B{ .base = C{ .typeId = B.typeId, .counter = 0 }} };
    //     }
    //     var j = list.len();
    //     print("j = {}\n", .{j});

    //     {
    //         var timer = std.time.Timer.start() catch unreachable;
    //         const t = timer.lap();
    //         i = 0;
    //         while (i < 1000) :(i += 1) {
    //             k = 0;
    //             while (k < j) :(k += 1) {
    //                 list.at(k).Update();
    //             }
    //         }
    //         print("elapsed_s = {}\n", .{ @intToFloat(f64, timer.read() - t) / std.time.ns_per_s });
    //     }

    //     {
    //         var timer = std.time.Timer.start() catch unreachable;
    //         const t = timer.lap();
    //         i = 0;
    //         var counter:isize = 0;
    //         while (i < j) :(i += 1) {
    //             counter += list.at(i).GetCounter();
    //         }
    //         print("{} elapsed_s = {}\n", .{ counter, @intToFloat(f64, timer.read() - t) / std.time.ns_per_s });
    //     }
    // }


    // var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    // defer arena.deinit();
    // const a = &arena.allocator;

    // var buffer:[4096]u8 = undefined;
    // const a = &std.heap.FixedBufferAllocator.init(&buffer).allocator;

    // {
    //     var list = List(i32).inita(a);
    //     defer list.deinit();
    //     const e = list.emplace();
    //     e.* = 123;
    //     print("{}\n", .{list.at(0).*});
    // }

    // {
    //     var list = List(List(List(Data))).inita(a);
    //     defer list.deinit();
    //     const d = list.emplace().emplace().emplace();
    //     d.writeFixed(@as(u32, 123));
    //     print("{}\n", .{list.at(0).at(0).at(0).at(0).*});
    // }
//}


// const C = packed struct {
//     typeId:isize,
//     counter:isize,
// };
// const A = struct {
//     const typeId:isize = 1;
//     base:C,
//     pub fn Update(this:*@This()) void {
//         this.base.counter += 1;
//     }
// };
// const B = struct {
//     const typeId:isize = 2;
//     base:C,
//     pub fn Update(this:*@This()) void {
//         this.base.counter -= 1;
//     }
// };
// const AB = union(enum) {
//     a:A,
//     b:B,
//     pub fn Update(this:*@This()) void {
//         switch(this.*) {
//             AB.a=>|*o|o.Update(),
//             AB.b=>|*o|o.Update(),
//         }
//     }
//     pub fn GetCounter(this:@This()) isize {
//         return switch(this) {
//             AB.a=>|o|o.base.counter,
//             AB.b=>|o|o.base.counter,
//         };
//     }
// };


    // var s = Shared(i32).inita(a);
    // defer s.deinit();
    // print("{}\n", .{ s.sharedCount() });

    // // test panic
    // //print("{}\n", .{ s.value() });

    // s.emplace(123);
    // print("{} {}\n", .{ s.sharedCount(), s.value() });

    // var t = s.clone();
    // defer t.deinit();

    // t.setValue(3456);
    // print("{} {} {} {}\n", .{ @ptrToInt(s.ptr), @ptrToInt(t.ptr), s.sharedCount(), s.value() });

    // xxxx(&t.clone());
    // print("{}\n", .{ s.sharedCount() });

    // var ss = List(Shared(i32)).inita(a);
    // defer ss.deinit();
    // ss.push(s.clone());