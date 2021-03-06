-- static lib
ObjMgr = {
    -- 创建 ObjMgr 实例
    New = function()
        local om = {}
        setmetatable(om, ObjMgr)
        return om
    end
    -- 记录 typeId 到 创建函数的映射
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
        self.objs = {}
        return self:Read()
    end
    -- 内部函数: 向 d 写入一个 "类". 格式: idx + typeId + content
    , Write = function(self, o)
        local d = self.d
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
        if r ~= 0 then return r end
        if n == 0 then return 0 end
        local m = self.m
        local len = #m, typeId
        if n == len + 1 then
            r, typeId = d:Rvu()
            if r ~= 0 then return r end
            if typeId == 0 then return 55 end
            local v = self[typeId].New()
            m[n] = v
            v:Read(self)
            return 0, v
        else
            if n > len then return 61 end
            return 0, m[n]
        end
    end
    -- 内部函数: 向 d 写入一个 "类"数组
    , WriteList = function(self, list)
        local d = self.d
        d:Wvu(#list)
        for _, v in ipairs(list) do
            self:Write(v)
        end
    end
    -- 内部函数: 从 d 读出一个 "类"数组
    , ReadList = function(self)
        local d = self.d
        local r, len = d:Rvu()
        if r ~= 0 then return r end
        if len > d:GetLeft() then return 78 end
        local t = {}
        for i = 1, len do
            r, t[i] = self:Read()
            if r ~= 0 then return r end
        end
        return 0, t
    end
    -- 内部函数: 向 d 写入一个 值数组. 需要传递 Data 的写函数名
    , WriteListValue = function(self, list, wfn)
        local d = self.d
        d:Wvu(#list)
        local wf = getmetatable(d)[wfn]
        for _, v in ipairs(list) do
            wf(d, v)
        end
    end
    -- 内部函数: 从 d 读出一个 值数组. 需要传递 Data 的写函数名
    , ReadListValue = function(self, rfn)
        local d = self.d
        local r, len = d:Rvu()
        if r ~= 0 then return r end
        if len > d:GetLeft() then return 100 end
        local rf = getmetatable(d)[rfn]
        local t = {}
        for i = 1, len do
            r, t[i] = rf(d)
            if r ~= 0 then return r end
        end
        return 0, t
    end
    -- 内部函数: 向 d 写入一个 可空(null代表)值 数组. 需要传递 Data 的写函数名
    , WriteListNullableValue = function(self, list, wfn)
        local d = self.d
        local mt = getmetatable(d)
        mt.Wvu(d, #list)
        local wf1 = mt.Wu8
        local wf2 = mt[wfn]
        for _, v in ipairs(list) do
            if v == null then
                wf1(d, 0)
            else
                wf1(d, 1)
                wf2(d, v)
            end
        end
    end
    -- 内部函数: 从 d 读出一个 可空(null代表)值 数组. 需要传递 Data 的写函数名
    , ReadListNullableValue = function(self, rfn)
        local d = self.d
        local mt = getmetatable(d)
        local r, len = mt.Rvu(d)
        if r ~= 0 then return r end
        if len > mt.GetLeft(d) then return 100 end
        local rf1 = mt.Ru8
        local rf2 = mt[rfn]
        local t = {}, n
        for i = 1, len do
            r, n = rf1(d)
            if r ~= 0 then return r end
            if n == 0 then
                t[i] = null
            else
                r, t[i] = rf2(d)
                if r ~= 0 then return r end
            end
        end
        return 0, t
    end
}
ObjMgr.__index = ObjMgr

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
        if r ~= 0 then return r end
        r, self.f = d:Rf()
        if r ~= 0 then return r end
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
        if r ~= 0 then return r end
        local d = om.d
        r, self.n = d:Ri32()
        if r ~= 0 then return r end
        r, self.s = d:Rstr()
        if r ~= 0 then return r end
        return 0
    end
}
Foo.__index = Foo

Bar = {
    typeName = "Bar"
    , typeId = 2
    , New = function(c)
        local o = c or {}
        o.f = null --Foo.New();
        if c == nil then
            setmetatable(o, Bar)
        end
        return o
    end
    , Write = function(self, om)
        -- no base
        local d = om.d
        om.Write(self.f)
    end
    , Read = function(self, om)
        local d = om.d, r
        r, self.f = om.Read()
        if r ~= 0 then return r end
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

--for i, v in pairs(XXXXX) do
--    print(i, v)
--end

XXX_RegisterTypesTo(om)

local d = NewXxData()
print("-------1111111----------")
print(d)
local f = Foo.New()
om:WriteTo(d, f)
print(d)
print("-------22222222----------")

local r, o = om:ReadFrom(d)
print("r, o = ",r, o)
for i, v in pairs(o) do
    print(i, v)
end

print("-------333----------")

--local b = Bar.New()
--b.f.

-- todo: List 读写测试
