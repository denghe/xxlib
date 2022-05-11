
require('g_net')
CodeGen_pkg_md5 ="#*MD5<f902fcd00b9558a3d09b8a7d71e2fe28>*#"

Generic_PlayerInfo = {
    typeName = "Generic_PlayerInfo",
    typeId = 14,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Generic_PlayerInfo)
        end
        o.id = 0 -- Int64
        o.username = "" -- String
        o.password = "" -- String
        o.nickname = "" -- String
        o.gold = 0 -- Int64
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- id
        r, self.id = d:Rvi64()
        if r ~= 0 then return r end
        -- username
        r, self.username = d:Rstr()
        if r ~= 0 then return r end
        -- password
        r, self.password = d:Rstr()
        if r ~= 0 then return r end
        -- nickname
        r, self.nickname = d:Rstr()
        if r ~= 0 then return r end
        -- gold
        r, self.gold = d:Rvi64()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- id
        d:Wvi64(self.id)
        -- username
        d:Wstr(self.username)
        -- password
        d:Wstr(self.password)
        -- nickname
        d:Wstr(self.nickname)
        -- gold
        d:Wvi64(self.gold)
    end
}
Generic_PlayerInfo.__index = Generic_PlayerInfo

--[[
return Pong{ value = Ping.ticks }
]]
Ping = {
    typeName = "Ping",
    typeId = 2062,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Ping)
        end
        o.ticks = 0 -- Int64
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- ticks
        r, self.ticks = d:Rvi64()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- ticks
        d:Wvi64(self.ticks)
    end
}
Ping.__index = Ping

Pong = {
    typeName = "Pong",
    typeId = 1283,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Pong)
        end
        o.ticks = 0 -- Int64
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- ticks
        r, self.ticks = d:Rvi64()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- ticks
        d:Wvi64(self.ticks)
    end
}
Pong.__index = Pong

--[[
push
]]
Lobby_Client_Scene = {
    typeName = "Lobby_Client_Scene",
    typeId = 801,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Lobby_Client_Scene)
        end
        o.playerInfo = null -- Shared<Generic.PlayerInfo>
        o.gamesIntro = "" -- String
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- playerInfo
        r, self.playerInfo = om:Read()
        if r ~= 0 then return r end
        -- gamesIntro
        r, self.gamesIntro = d:Rstr()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- playerInfo
        om:Write(self.playerInfo)
        -- gamesIntro
        d:Wstr(self.gamesIntro)
    end
}
Lobby_Client_Scene.__index = Lobby_Client_Scene

--[[
return Generic.Error || Generic.Success{ value = id } + Push Scene
]]
Client_Lobby_Login = {
    typeName = "Client_Lobby_Login",
    typeId = 501,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Client_Lobby_Login)
        end
        o.username = "" -- String
        o.password = "" -- String
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- username
        r, self.username = d:Rstr()
        if r ~= 0 then return r end
        -- password
        r, self.password = d:Rstr()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- username
        d:Wstr(self.username)
        -- password
        d:Wstr(self.password)
    end
}
Client_Lobby_Login.__index = Client_Lobby_Login

--[[
push
]]
Generic_Register = {
    typeName = "Generic_Register",
    typeId = 11,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Generic_Register)
        end
        o.id = 0 -- UInt32
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- id
        r, self.id = d:Rvu32()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- id
        d:Wvu32(self.id)
    end
}
Generic_Register.__index = Generic_Register

Generic_Success = {
    typeName = "Generic_Success",
    typeId = 12,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Generic_Success)
        end
        o.value = 0 -- Int64
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- value
        r, self.value = d:Rvi64()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- value
        d:Wvi64(self.value)
    end
}
Generic_Success.__index = Generic_Success

Generic_Error = {
    typeName = "Generic_Error",
    typeId = 13,
    Create = function(o)
        if o == nil then
            o = {}
            setmetatable(o, Generic_Error)
        end
        o.number = 0 -- Int64
        o.message = "" -- String
        return o
    end,
    Read = function(self, om)
        local d = om.d
        local r, n
        -- number
        r, self.number = d:Rvi64()
        if r ~= 0 then return r end
        -- message
        r, self.message = d:Rstr()
        if r ~= 0 then return r end
        return 0
    end,
    Write = function(self, om)
        local d = om.d
        -- number
        d:Wvi64(self.number)
        -- message
        d:Wstr(self.message)
    end
}
Generic_Error.__index = Generic_Error

local o = ObjMgr
o.Register(Generic_PlayerInfo)
o.Register(Ping)
o.Register(Pong)
o.Register(Lobby_Client_Scene)
o.Register(Client_Lobby_Login)
o.Register(Generic_Register)
o.Register(Generic_Success)
o.Register(Generic_Error)