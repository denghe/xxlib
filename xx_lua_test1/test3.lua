require 'gen2'

CodeGen_shared.Register()

local om = ObjMgr.Create()
local d = NewXxData()

--[[
local a = A.Create()
a.id = 1
a.nick = "asdf"
a.parent = a
table.insert(a.children, a)

om:WriteTo(d, a)
print(d)

local r, x = om:ReadFrom(d)
print(r)
print(x)
print(x.id)
print(x.nick)
print(x.parent)
print(x.children[1])
print(#x.children)
]]

a = B.Create()
a.id = 1
a.nick = "asdf"
a.parent = a
table.insert(a.children, a)
a.data:Fill( 1,2,3,4,5 )
a.c.x = 1.2
a.c.y = 2.3
table.insert(a.c.targets, a)
a.c2 = a.c
table.insert(a.c3, a.c)

d:Clear()
om:WriteTo(d, a)
print(d)

r, x = om:ReadFrom(d)
print("r = "..r)
print(x)
print(x.id)
print(x.nick)
print(x.parent)
print(x.children[1])
print(#x.children)
print(x.data)
print(x.c.x)
print(x.c.y)
print(#x.c.targets)
print(x.c2)
print(x.c2.x)
print(x.c2.y)
print(#x.c2.targets)
print(x.c2.targets[1])
print(x.c3[1].x)
print(x.c3[1].y)
print(#x.c3[1].targets)
print(x.c3[1].targets[1])
