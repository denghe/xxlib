-- lua 模拟 class 考虑不支持继承. 直接所有成员展开. 也没必要弄 metatable, 直接带上序列化函数.

Foo = {
    typeName = "Foo",
    typeId = 1817,
    New = function()
        local o = {}
        o.n = 123;
        o.s = "asdf";

        o.__proto = Foo
        o.Read = function(d)
            local r
            r, o.n = d:Ri32()
            if r ~= 0 then return r end
            r, o.s = d:Rstr()
            if r ~= 0 then return r end
            return 0
        end
        o.Write = function(d)
            d:Wi32(o.n)
            d:Wstr(o.s)
        end

        return o
    end
}
-- ObjMgr.Register( Foo )   这步将 typeId 和 Table 建立关联
