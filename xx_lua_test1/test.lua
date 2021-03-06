require 'gen'

CodeGen_xxx.Register()
-- more register here

local om = ObjMgr.Create()

print("-------1111111----------")
local d = NewXxData()
local fb = FooBase.Create()
om:WriteTo(d, fb)
print(d:GetAll())
print(d)
local r, o = om:ReadFrom(d)
print("r, o = ", r, o)
print(d:GetAll())

print("-------22222222----------")
d:Clear()
local f = Foo.Create()
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
local b = Bar.Create()
table.insert(b.foos, Foo.Create())
table.insert(b.foos, Foo.Create())
b.ints = { 3, 4, 5 }
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
for k, v in pairs(o.foos) do
    print(k, v)
    --for _, v2 in ipairs(v) do
    --    print(v2)
    --end
end
for _, v in ipairs(o.foos) do
    for k, v2 in pairs(v) do
        print(k, v2)
    end
end
for _, v in ipairs(o.ints) do
    print(v)
end

print("-------55555----------")
