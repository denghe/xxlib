-- static lib
ObjMgr = {
    -- 创建 ObjMgr 实例
    New = function()
        local om = {}
        setmetatable(om, ObjMgr)
        return om
    end
    -- 记录 typeId 到 元表 的映射
, Register = function(self, o)
        self[o.typeId] = o
    end
    -- 入口函数: 始向 d 写入一个 "类"
, WriteTo = function(self, d, o)
        self.d = d
        self.m = {}
        self:Write(o)
    end
    -- 入口函数: 开始从 d 读出一个 "类" 并返回 r, o    ( r == 0 表示成功 )
, ReadFrom = function(self, d)
        self.d = d
        self.m = {}
        return self:Read()
    end
    -- 内部函数: 向 d 写入一个 "类". 格式: idx + typeId + content
, Write = function(self, o)
        print("self = ", self)
        local d = self.d
        print("d = ", d)
        print("self.m = ", self.m)
        if o == null or o == nil then
            d:Wu8(0)
        else
            local m = self.m
            local n = m[o]
            if n == nil then
                n = #m + 1
                m[o] = n
                d:Wvu(n)
                d:Wvu(getmetatable(o).typeId)
                o:Write(self)
            else
                d:Wvu(n)
            end
        end
    end
    -- 内部函数: 从 d 读出一个 "类" 并返回 r, o    ( r == 0 表示成功 )
, Read = function(self)
        local d = self.d
        local r, n = d:Rvu()
        if r ~= 0 then
            return r
        end
        if n == 0 then
            return 0, null
        end
        local m = self.m
        local len = #m, typeId
        if n == len + 1 then
            r, typeId = d:Rvu()
            if r ~= 0 then
                return r
            end
            if typeId == 0 then
                return 62
            end
            local v = self[typeId].New()
            m[n] = v
            v:Read(self)
            return 0, v
        else
            if n > len then
                return 70
            end
            return 0, m[n]
        end
    end
    -- 内部函数: 向 d 写入一个 "类"数组 或 子集合( 可能嵌套 ). fn 为最后一级的操作函数名, level 为级数
, WriteArray = function(self, a, fn, level)
        local d = self.d
        d:Wvu(#a)
        if #a == 0 then
            return
        end
        if level == nil or level == 1 then
            if fn == nil then
                for _, v in ipairs(a) do
                    self:Write(v)
                end
            else
                local wf = getmetatable(d)[fn]
                for _, v in ipairs(a) do
                    wf(d, v)
                end
            end
        else
            for _, v in ipairs(a) do
                self:WriteArray(v, fn, level - 1)
            end
        end
    end
    -- 内部函数: 从 d 读出一个 "类"数组 或 子集合( 可能递归 ). fn 为最后一级的操作函数名, level 为级数
, ReadArray = function(self, fn, level)
        local d = self.d
        local r, len = d:Rvu()
        if r ~= 0 then
            return r
        end
        if len > d:GetLeft() then
            print("len > d:GetLeft()", len, d:GetAll())
            return 107
        end
        local t = {}
        if level == nil or level == 1 then
            if fn == nil then
                for i = 1, len do
                    r, t[i] = self:Read()
                    if r ~= 0 then
                        return r
                    end
                end
            else
                local f = getmetatable(d)[fn]
                for i = 1, len do
                    r, t[i] = f(d)
                    if r ~= 0 then
                        return r
                    end
                end
            end
        else
            for i = 1, len do
                r, t[i] = self:ReadArray(fn, level - 1)
                if r ~= 0 then
                    return r
                end
            end
        end
        return 0, t
    end
}
ObjMgr.__index = ObjMgr

-- 为 table 附加精简的集合操作函数
List = {
    New = function(...)
        t = { ... }
        setmetatable(t, List)
        return t
    end
, Add = function(self, ...)
        local args = { ... }
        for i, v in pairs(args) do
            table.insert(self, v)
        end
    end
, SwapRemoveAt = function(self, idx)
        local siz = #self
        assert(idx > 0 and idx <= siz)
        if idx < siz then
            self[idx] = self[siz]
        end
        self[siz] = nil
    end
}
List.__index = List

-- code gens
FooBase = {
    typeName = "FooBase"
, typeId = 3
, New = function(c)
        local o = c or {}
        o.d = 1.234
        o.f = 1.2
        if c == nil then
            setmetatable(o, FooBase)
        end
        return o
    end
, Write = function(self, om)
        local d = om.d
        -- todo: 向下兼容
        d:Wd(self.d)
        d:Wf(self.f)
        -- todo: 向下兼容
    end
, Read = function(self, om)
        local d = om.d, r
        -- todo: 向下兼容
        r, self.d = d:Rd()
        if r ~= 0 then
            return r
        end
        r, self.f = d:Rf()
        if r ~= 0 then
            return r
        end
        -- todo: 向下兼容
        return 0
    end
}
FooBase.__index = FooBase

Foo = {
    typeName = "Foo"
, typeId = 1
, New = function(c)
        local o = c or {}
        FooBase.New(o)                          -- call base func
        o.n = 123
        o.s = "asdf"
        if c == nil then
            setmetatable(o, Foo)
        end
        return o
    end
, Write = function(self, om)
        FooBase.Write(self, om)                 -- call base func
        local d = om.d
        d:Wi32(self.n)
        d:Wstr(self.s)
    end
, Read = function(self, om)
        local r = FooBase.Read(self, om)        -- call base func
        if r ~= 0 then
            return r
        end
        local d = om.d
        r, self.n = d:Ri32()
        if r ~= 0 then
            return r
        end
        r, self.s = d:Rstr()
        if r ~= 0 then
            return r
        end
        return 0
    end
}
Foo.__index = Foo

Bar = {
    typeName = "Bar"
, typeId = 2
, New = function(c)
        local o = c or {}
        o.foos = {}
        o.ints = {}
        if c == nil then
            setmetatable(o, Bar)
        end
        return o
    end
, Write = function(self, om)
        -- no base
        local d = om.d

        om:WriteArray(self.foos)
        om:WriteArray(self.ints, "Wvi")
    end
, Read = function(self, om)
        local d = om.d, r

        r, self.foos = om:ReadArray()
        if r ~= 0 then
            return r
        end
        r, self.ints = om:ReadArray("Rvi")
        if r ~= 0 then
            return r
        end

        return 0
    end
}
Bar.__index = Bar

-- 可能带前缀
XXX_RegisterTypesTo = function(om)
    om:Register(FooBase)
    om:Register(Foo)
    om:Register(Bar)
end

---------------------------------------------

-- hand write
local om = ObjMgr.New()
XXX_RegisterTypesTo(om)

print("-------1111111----------")
local d = NewXxData()
local fb = FooBase.New()
om:WriteTo(d, fb)
print(d:GetAll())
print(d)
local r, o = om:ReadFrom(d)
print("r, o = ", r, o)
print(d:GetAll())

print("-------22222222----------")
d:Clear()
local f = Foo.New()
om:WriteTo(d, f)
print(d:GetAll())
print(d)
r, o = om:ReadFrom(d)
print("r, o = ", r, o)
for i, v in pairs(o) do
    print(i, v)
end
print(d:GetAll())


print("-------333----------")
d:Clear()
local b = Bar.New()
table.insert(b.foos, FooBase.New())
--table.insert(b.foos, Foo.New())
--b.ints = {3,4,5}
for i, v in pairs(b) do
    print(i, v)
end
om:WriteTo(d, b)
print(d)
print(d:GetAll())

r, o = om:ReadFrom(d)
print("r, o = ", r, o)
print(d:GetAll())

for k, v in pairs(o) do
    print(k, v)
end
for _, v in ipairs(o.ints) do
    print(v)
end

print("-------55555----------")
