require "objmgr"

CodeGen_shared = {
    md5 = "#*MD5<a817d51e475ac1932bd31f0f941f65e7>*#",
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
        local d = om.d, o, r
        -- id
        r, self.id = d:Ri32()
        if r ~= 0 then return r end
        -- nick
        r, self.nick = om:Rnstr()
        if r ~= 0 then return r end
        -- parent
        r, self.parent = om:Read()
        if r ~= 0 then return r end
        -- children
        r, len = d:Rvu()
        if len > d:GetLeft() then return -1 end
        o = {}
        self.children = o
        for i = 1, len do
            r, o[i] = om:Read()
            if r ~= 0 then return r end
        end
        if r ~= 0 then return r end
    end,
    Write = function(self, om)
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
        local d = om.d, o, r
        -- base read
        r = A.Read(self, om)
        if r ~= 0 then
            return r
        end
        -- data
        r, self.data = om:Rdata()
        if r ~= 0 then return r end
        -- c
        self.c = C.Create();
        r = self.c:Read(om)
        if r ~= 0 then return r end
        -- c2
        r, o = d:Ru8();
        if r ~= 0 then return r end;
        if o == 0 then
            self.c2 = null
        else
            self.c2 = C.Create();
            r = self.c2:Read(om)
        end
        if r ~= 0 then return r end
        -- c3
        r, len = d:Rvu()
        if len > d:GetLeft() then return -1 end
        o = {}
        self.c3 = o
        for i = 1, len do
            r, o = d:Ru8();
            if r ~= 0 then return r end;
            if o == 0 then
                o[i] = null
            else
                o[i] = C.Create();
                r = o[i]:Read(om)
            end
            if r ~= 0 then return r end
        end
        if r ~= 0 then return r end
    end,
    Write = function(self, om)
    end
}
B.__index = B

--[[
我怕擦
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
        local d = om.d, o, r
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
    end
}
C.__index = C
