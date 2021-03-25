require "objmgr"

CodeGen_shared = {
    md5 = "#*MD5<115d3d5cd445941f39210ad7c08d16ff>*#",
    Register = function()
        local o = ObjMgr
        o.Register(A)
        o.Register(B)
    end
}

A = {
    typeName = "A",
    typeId = 1,
    Create = function(c)
        if c == nil then
            c = {}
            setmetatable(c, A)
        end
        c.id = 0 -- Int32
        c.nick = null -- Nullable<String>
        c.parent = null -- Weak<A>
        c.children = {} -- List<Shared<A>>
        return o
    end,
    Read = function(self, om)
        local d = om.d, r, o, len, e
        -- compatible handle
        r, len = d:Ru32()
        if r ~= 0 then return r end
        e = d:GetOffset() - 4 + len
        -- id
        if d:GetOffset() >= e then
            self.id = 0
        else
            r, self.id = d:Ri32()
            if r ~= 0 then return r end
        end
        -- nick
        if d:GetOffset() >= e then
            self.nick = null
        else
            r, self.nick = om:Rnstr()
            if r ~= 0 then return r end
        end
        -- parent
        if d:GetOffset() >= e then
            self.parent = null
        else
            r, self.parent = om:Read()
            if r ~= 0 then return r end
        end
        -- children
        if d:GetOffset() >= e then
            self.children = {}
        else
            r, len = d:Rvu()
            if len > d:GetLeft() then return -1 end
            o = {}
            self.children = o
            for i = 1, len do
                r, o[i] = om:Read()
                if r ~= 0 then return r end
            end
            if r ~= 0 then return r end
        end
        -- compatible handle
        if d:GetOffset() > e then return -1 end
        d:SetOffset(e)
    end,
    Write = function(self, om)
        local d = om.d, o, len
        -- compatible handle
        local bak = d:Wj(4)
        -- id
        d:Wi32(self.id)
        -- nick
        d:Wnstr(self.nick)
        -- parent
        om:Write(self.parent)
        -- children
        o = self.children
        len = #o
        d:Wvu(len)
        for i = 1, len do
            om:Write(o[i])
        end
        -- compatible handle
        d:Wu32_at(bak, d:GetLen() - bak);
    end
}
A.__index = A

B = {
    typeName = "B", -- : A
    typeId = 2,
    Create = function(c)
        if c == nil then
            c = {}
            setmetatable(c, B)
        end
        A.Create(c)
        c.data = NewXxData() -- XxData
        c.c = C.Create() -- C
        c.c2 = null -- Nullable<C>
        c.c3 = {} -- List<Nullable<C>>
        return o
    end,
    Read = function(self, om)
        local d = om.d, r, o
        -- base read
        r = A.Read(self, om)
        if r ~= 0 then return r end
        -- data
        r, self.data = om:Rdata()
        if r ~= 0 then return r end
        -- c
        self.c = C.Create(); r = self.c:Read(om)
        if r ~= 0 then return r end
        -- c2
        r, o = d:Ru8(); if r ~= 0 then return r end; if o == 0 then self.c2 = null else self.c2 = C.Create(); r = self.c2:Read(om) end
        if r ~= 0 then return r end
        -- c3
        r, len = d:Rvu()
        if len > d:GetLeft() then return -1 end
        o = {}
        self.c3 = o
        for i = 1, len do
            r, o = d:Ru8(); if r ~= 0 then return r end; if o == 0 then o[i] = null else o[i] = C.Create(); r = o[i]:Read(om) end
            if r ~= 0 then return r end
        end
        if r ~= 0 then return r end
    end,
    Write = function(self, om)
        local d = om.d, o, len
        -- base read
        A.Write(self, om)
        -- data
        d:Wdata(self.data)
        -- c
        self.c:Write(om)
        -- c2
        if self.c2 == null then d:Wu8(0) else d:Wu8(1); self.c2:Write(om) end
        -- c3
        o = self.c3
        len = #o
        d:Wvu(len)
        for i = 1, len do
            if o[i] == null then d:Wu8(0) else d:Wu8(1); o[i]:Write(om) end
        end
    end
}
B.__index = B

--[[
asdfasdf
]]
C = {
    typeName = "C",
    Create = function(c)
        if c == nil then
            c = {}
            setmetatable(c, C)
        end
        c.x = 0 -- Single
        c.y = 0 -- Single
        c.targets = {} -- List<Weak<A>>
        return o
    end,
    Read = function(self, om)
        local d = om.d, r, o
        -- x
        r, self.x = d:Rf()
        if r ~= 0 then return r end
        -- y
        r, self.y = d:Rf()
        if r ~= 0 then return r end
        -- targets
        r, len = d:Rvu()
        if len > d:GetLeft() then return -1 end
        o = {}
        self.targets = o
        for i = 1, len do
            r, o[i] = om:Read()
            if r ~= 0 then return r end
        end
        if r ~= 0 then return r end
    end,
    Write = function(self, om)
        local d = om.d, o, len
        -- x
        d:Wf(self.x)
        -- y
        d:Wf(self.y)
        -- targets
        o = self.targets
        len = #o
        d:Wvu(len)
        for i = 1, len do
            om:Write(o[i])
        end
    end
}
C.__index = C
