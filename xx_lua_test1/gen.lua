-- 模拟生成物
require 'objmgr'

CodeGen_xxx = {
    md5 = "xxxxxxxxxxxxxxxxxx",
    Register = function()
        local o = ObjMgr
        o.Register(FooBase)
        o.Register(Foo)
        o.Register(Bar)
    end
}

FooBase = {
    typeName = "FooBase"
, typeId = 3
, Create = function(c)
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
        local bak = d:Wj(4)

        d:Wd(self.d)
        d:Wf(self.f)

        d:Wu32_at(bak, d:GetLen() - bak)
        print(222)
    end
, Read = function(self, om)
        local d = om.d        -- 如果非兼容模式 则 r 声明在这句

        local r, siz = d:Ru32()
        if r ~= 0 then return r end
        local eo = d:GetOffset() - 4 + siz

        if d:GetOffset() >= eo then
            self.d = 1.234              -- 填默认值
        else
            r, self.d = d:Rd()
            if r ~= 0 then
                return r
            end
        end

        if d:GetOffset() >= eo then
            self.f = 1.2                -- 填默认值
        else
            r, self.f = d:Rf()
            if r ~= 0 then return r end
        end

        if d:GetOffset() > eo then return -1 end
        d:SetOffset(eo)
        return 0
    end
}
FooBase.__index = FooBase

Foo = {
    typeName = "Foo"
, typeId = 1
, Create = function(c)
        local o = c or {}
        FooBase.Create(o)                          -- call base func
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
        local r
        r = FooBase.Read(self, om)        -- call base func
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
, Create = function(c)
        local o = c or {}
        o.foos = {} -- List<Foo>
        o.ints = {} -- List<int>
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

