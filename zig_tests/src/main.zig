const std = @import("std");
const native_endian = @import("builtin").target.cpu.arch.endian();
const print = std.debug.print;
const assert = std.debug.assert;

pub fn main() void {
    // var gpa = std.heap.GeneralPurposeAllocator(.{}){}; // .{ .enable_memory_limit = true }){};
    // defer _ = gpa.deinit();
    // //gpa.setRequestedMemoryLimit(512);
    // const a = &gpa.allocator;

    // var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    // defer arena.deinit();
    // const a = &arena.allocator;

    var buffer: [4096]u8 = undefined;
    const a = &std.heap.FixedBufferAllocator.init(&buffer).allocator;

    {
        var list = List(i32).inita(a);
        defer list.deinit();
        const e = list.emplace();
        e.* = 123;
        print("{}\n", .{list.at(0).*});
    }

    // {
    //     var list = List(List(List(Data))).inita(a);
    //     defer list.deinit();
    //     const d = list.emplace().emplace().emplace();
    //     d.writeFixed(@as(u32, 123));
    //     print("{}\n", .{list.at(0).at(0).at(0).at(0).*});
    // }
}

pub fn Array(comptime T: type) type {
    return struct {
        const This = @This();
        const tIsStruct = @typeInfo(T) == .Struct;
        const tHasInit = tIsStruct and @hasDecl(T, "init");
        const tHasInita = tIsStruct and @hasDecl(T, "inita");
        const tHasDeinit = tIsStruct and @hasDecl(T, "deinit");

        buf: ?[]T,
        len: usize,
        allo: *std.mem.Allocator,
        pub fn cap(this: This) usize {
            return if (this.buf) |b| b.len else 0;
        }
        
        pub fn inita(allo: *std.mem.Allocator) This {
            return .{
                .buf = null,
                .len = 0,
                .allo = allo,
            };
        }

        pub fn deinit(this: *This) void {
            if (this.buf) |b| {
                this.clear();
                this.allo.free(b);
            }
        }

        pub fn clear(this: *This) void {
            if (this.buf) |b| {
                if (tHasDeinit) {
                    var i: usize = 0;
                    while (i < this.len) : (i += 1) {
                        this.buf.?[i].deinit();
                    }
                }
                this.len = 0;
            }
        }

        pub fn reserve(this: *This, newCap: usize) void {
            @setCold(true);
            if (this.buf) |b| {
                if (b.len >= newCap) return;
                var c = b.len * 2;
                while (c < newCap) : (c *= 2) {}
                this.buf = this.allo.realloc(b, c) catch unreachable;
            } else {
                this.buf = this.allo.alloc(T, newCap) catch unreachable;
                // preload from virtual memory
                var i : usize = 0;
                while (i < newCap * @sizeOf(T)) : (i += 4096) {
                    @ptrCast([*]u8, &this.buf.?[0])[i] = 0;
                }
            }
        }

        pub fn at(this: This, idx: usize) *T {
            return &this.buf.?[idx];
        }
    };
}

pub fn List(comptime T: type) type {
    return struct {
        const This = @This();
        const Base = Array(T);
        base: Base,
        pub fn cap(this: This) usize { return this.base.cap(); }
        pub fn len(this: This) usize { return this.base.len; }
        pub fn reserve(this: *This, newCap: usize) void { this.base.reserve(newCap); }
        pub fn inita(allo: *std.mem.Allocator) This { return .{ .base = Base.inita(allo) }; }
        pub fn deinit(this: *This) void { this.base.deinit(); }
        pub fn clear(this: *This) void { this.base.clear(); }
        pub fn at(this: *This, idx: usize) *T { return this.base.at(idx); }

        pub fn emplace(this: *This) *T {
            const a = &this.base;
            const newCap = a.len + 1;
            if (a.cap() < newCap) {
                a.reserve(newCap);
            }
            const r = &a.buf.?[a.len];
            a.len += 1;
            if (Base.tHasInita) {
                r.* = T.inita(a.allo);
            }
            else if (Base.tHasInit) {
                r.* = T.init();
            } // ...
            return r;
        }
    };
}

pub const Data = struct {
    const T = u8;
    const This = @This();
    const Base = Array(T);
    base: Base,
    pub fn cap(this: This) usize { return this.base.cap(); }
    pub fn len(this: This) usize { return this.base.len; }
    pub fn reserve(this: *This, newCap: usize) void { this.base.reserve(newCap); }
    pub fn inita(allo: *std.mem.Allocator) This { return .{ .base = Base.inita(allo) }; }
    pub fn deinit(this: *This) void { this.base.deinit(); }
    pub fn clear(this: *This) void { this.base.clear(); }
    pub fn at(this: *This, idx: usize) *T { return this.base.at(idx); }

    pub fn writeFixed(this: *This, v: anytype) void {
        const t = @TypeOf(v);
        switch (@typeInfo(t)) {
             .Int, .Float => {
                if (native_endian == .Big) {
                    v = @byteSwap(t, v);
                }
            },
            else => {
                @compileError("writeFixed unsupported type: " ++ @typeName(@TypeOf(v)));
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
    base: Base,
    pub fn cap(this: This) usize { return this.base.cap(); }
    pub fn len(this: This) usize { return this.base.len; }
    pub fn reserve(this: *This, newCap: usize) void { this.base.reserve(newCap); }
    pub fn inita(allo: *std.mem.Allocator) This { return .{ .base = Base.inita(allo) }; }
    pub fn deinit(this: *This) void { this.base.deinit(); }
    pub fn clear(this: *This) void { this.base.clear(); }
    pub fn at(this: *This, idx: usize) *T { return this.base.at(idx); }

    pub fn append(this: *This, v: anytype) void {
        // const a = &this.base;
        // var siz = std.fmt.count(comptime fmt: []const u8, args: anytype)
        // const newCap = a.len + siz;
        // if (a.cap() < newCap) {
        //     a.reserve(newCap);
        // }
        // var buf = @ptrCast([*]u8, .....
        switch (@typeInfo(T)) {
            .Array => |info| {
                //std.fmt.bufPrint(buf: []u8, "{s}", v);
            },
            else => {
                //std.fmt.bufPrint(buf: []u8, "{}", v);
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
    //     while (i < 1000000000) : (i += 1) {
    //         d.writeFixed(@as(u32, 123));
    //     }
    //     print("elapsed_s = {}\n", .{ @intToFloat(f64, timer.read() - t) / std.time.ns_per_s });
    //     print("d.len = {}, d.cap = {}\n", .{ d.len(), d.cap() });
    //     //print("d.buf = {any}\n", .{ d.buf });
    // }
    




// pub const Data = struct {
//     const This = @This();
//     buf: ?[]u8,
//     len: usize,
//     a: *std.mem.Allocator,
//     pub fn cap(this: This) usize {
//         return if (this.buf) |b| b.len else 0;
//     }
    
//     pub fn inita(a: *std.mem.Allocator) This {
//         return .{
//             .buf = null,
//             .len = 0,
//             .a = a,
//         };
//     }

//     pub fn deinit(this: *This) void {
//         if (this.buf) |b| {
//             this.a.free(b);
//         }
//     }

//     pub fn round2n(n: usize) usize {
//         var rtv = @as(usize, 1) << @intCast(u6, @bitSizeOf(usize) - 1 - @clz(usize, n));
//         return if (rtv == n) n else rtv << 1;
//     }
//     pub fn reserve(this: *This, newCap: usize) void {
//         @setCold(true);
//         if (this.buf) |b| {
//             if (b.len < newCap) {
//             this.buf = this.a.realloc(b, round2n(newCap)) catch unreachable;
//             }
//         } else {
//             this.buf = this.a.alloc(u8, round2n(newCap)) catch unreachable;
//             var i : usize = 0;
//             while (i < newCap) : (i += 4096) {
//                 this.buf.?[i] = 0;
//             }
//         }
//     }
    
//     pub fn writeFixed(this: *This, v: anytype) void {
//         const t = @TypeOf(v);
//         switch (@typeInfo(t)) {
//              .Int, .Float => {
//                 if (native_endian == .Big) {
//                     v = @byteSwap(t, v);
//                 }
//             },
//             else => {
//                 @compileError("writeFixed unsupported type: " ++ @typeName(@TypeOf(v)));
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





    // pub fn round2n(n: usize) usize {
    //     var rtv = @as(usize, 1) << @intCast(u6, @bitSizeOf(usize) - 1 - @clz(usize, n));
    //     return if (rtv == n) n else rtv << 1;
    // }

    // pub const Error = error{
    //     OutOfMemory,
    //     InvalidRange,
    // };
    // pub fn reserve(this: *This, newCap: usize) Error!void {
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
    //pub fn writeFixed(this: *This, v: anytype) Error!void {
    //     const t = @TypeOf(v);
    //     const ti = @typeInfo(t);
    //     switch (ti) {
    //          .Int, .Float => {
    //             if (native_endian == .Big) {
    //                 v = @byteSwap(t, v);
    //             }
    //         },
    //         else => {
    //             @compileError("writeFixed unsupported type: " ++ @typeName(@TypeOf(v)));
    //         },
    //     }
    //     try this.reserve(this.len + @sizeOf(t));
    //     @memcpy(@ptrCast([*]u8, &this.buf.?[this.len]), @ptrCast([*] const u8, &v), @sizeOf(t));
    //     this.len += @sizeOf(t);
    // }




















//const fs = std.fs;

// pub fn pressEnterContinue() void {
//     print("press Enter to continue...", .{});
//     var line_buf: [1]u8 = undefined;
//     _ = std.io.getStdIn().read(&line_buf) catch unreachable;
// }

    //var buffer: [1024*1024]u8 = undefined;
    //var fba = std.heap.FixedBufferAllocator.init(&buffer);
    //print("fba.end_index = {}\n", .{ fba.end_index });
    //const a = &fba.allocator;

    // var buf : [ fs.MAX_PATH_BYTES:0 ] u8 = undefined;
    // var exedir = fs.selfExeDirPath(&buf) catch unreachable;
    // print("exedir = {s}\n", .{ exedir });

    //print("fba.end_index = {}\n", .{ fba.end_index });

    //defer{pressEnterContinue();}
    //print("fs.MAX_PATH_BYTES = {}\n", .{ fs.MAX_PATH_BYTES });


// const std = @import("std");
// const print = std.debug.print;

// pub fn isUnsigned(comptime T: type) bool {  // callconv(.Inline)
//     for(@typeName(T)) |c, i| {
//         if (c == 'u' or c == 'U') 
//             return true;
//     }
//     return false;
// }

// const myuint256 = struct {
//     a : []u8
// };

// pub fn main() void {
//     print("{} {} {}", .{ comptime isUnsigned(i32), comptime isUnsigned(u32), comptime isUnsigned(myuint256) });
//     var x : u32 = std.math.maxInt(u32);
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
//     a: i32
// };

// const S2 = struct {
//     b: i64
// };

// const S3 = S1 || S2;
