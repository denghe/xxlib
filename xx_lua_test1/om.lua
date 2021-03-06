-- static lib
ObjMgr = {
    New = function()
        local om = {}
        -- 记录 typeId 到 创建函数的映射
        om.Register = function(o)
            local mt = getmetatable(o)
            om[mt.typeId] = mt.New
        end
        -- 将 o 序列化到 d ( xxData )
        om.WriteTo = function(d, o)
            om.d = d
            om.objs = {}
            om.Write(o)
        end
        -- 从 d 读出一个 "类" 并返回 int, o    ( int == 0 表示成功 )
        om.ReadFrom = function(d)
            om.d = d
            om.ptrs = {}
            -- todo
            return -1
        end
        -- 以 offset + typeId + content[4 bytes len] 的格式序列化一个"类"
        om.Write = function(o)
            -- todo  o:Write(om)
        end
        -- todo
        om.Read = function()

        end
        return om
    end
}

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
    , Write = function(om)
        local d = om.d
        d:Wd(self.d)
        d:Wf(self.f)
    end
    , Read = function(om)
        local d = om.d, r
        r, self.d = d:Rd()
        if r ~= 0 then return r end
        r, self.f = d:Rf()
        if r ~= 0 then return r end
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
    , Write = function(om)
        FooBase.Write(self, om)                 -- call base func
        local d = om.d
        d:Wi32(self.n)
        d:Wstr(self.s)
    end
    , Read = function(om)
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
        -- 填充初始值. 基类成员就地展开?
        o.f = Foo.New();
        if c == nil then
            setmetatable(o, Bar)
        end
        return o
    end
    , Write = function(om)
        -- no base
        local d = om.d
        om.Write(self.f)
    end
    , Read = function(om)
        local d = om.d, r
        r, self.n = d:Ri32()
        if r ~= 0 then return r end
        r, self.s = d:Rstr()
        if r ~= 0 then return r end
        return 0
    end,
}
Bar.__index = Bar

-- 可能带前缀
FooBar_RegisterTypesTo = function(om)
    om.Register(Foo)
    om.Register(Bar)
end

---------------------------------------------
-- hand write
local om = ObjMgr.New()
FooBar_RegisterToObjMgr(om)

local d = NewXxData()
local f = Foo.New()
om.WriteTo(d, f)
print(d)
