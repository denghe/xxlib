-- 包管理器( 读写, ... )
ObjMgr = {
    -- 创建 ObjMgr 实例( 静态函数 )
    Create = function()
        local om = {}
        setmetatable(om, ObjMgr)
        return om
    end
    -- 记录 typeId 到 元表 的映射( 静态函数 )
, Register = function(o)
        ObjMgr[o.typeId] = o
    end
    -- 入口函数: 始向 d 写入一个 "类"
, WriteTo = function(self, d, o)
        self.d = d
        self.m = {}
        self.m.len = 0
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
        local d = self.d
        if o == null or o == nil then
            d:Wu8(0)
        else
            local m = self.m
            local n = m[o]
            if n == nil then
                n = m.len + 1
                m.len = n
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
                return 64
            end
            local v = ObjMgr[typeId].Create()
            m[n] = v
            v:Read(self)
            return 0, v
        else
            if n > len then
                return 72
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
            return 109
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
