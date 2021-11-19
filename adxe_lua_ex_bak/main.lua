-- �� lua �� release ģʽ��Ҳ�����
print = release_print

-- ������ʼ����������
local scene = cc_Scene_create()
cc_runWithScene(scene)

-- ����һ������
local spr = cc_Sprite_create()
spr:setTexture("HelloWorld.png")
scene:addChild(spr)

--[[
local for10000000 = function()
    for i = 1, 10000000 do
    end
end
local beginTime = os.clock()
for10000000()
release_print( "for10000000 elapsed time = ", os.clock() - beginTime)

local setGetPos10000000 = function(tar)
    for i = 1, 10000000 do
	    tar:setPosition(tar:getPosition())
    end
end
local beginTime = os.clock()
setGetPos10000000(spr)
print("setGetPos10000000 elapsed time = ", os.clock() - beginTime)

local setColor10000000 = function(tar)
    for i = 1, 10000000 do
	    tar:setColor(255, 255, 255)
    end
end
local beginTime = os.clock()
setColor10000000(spr)
print("setColor10000000 elapsed time = ", os.clock() - beginTime)
]]

spr:setPosition(300, 300)

local r, g, b = spr:getColor()
g = 0;
spr:setColor(r, g, b)

-- �����г������ñ�ø�ֱ��
table_unpack = table.unpack
coroutine_create = coroutine.create
coroutine_status = coroutine.status
resume = coroutine.resume
yield = coroutine.yield

-- �򵥵�����ṹ, �����ڵ�����ʲ�����ɾ�� if #t > 0 then for i = #t, 1, -1 do ...... end end
array_create = function()
	local t = {}
	t.add = function(o)
		t[#t + 1] = o
	end
	t.remove = function(i)
		local c = #t
		assert(i > 0 and i <= c)
		if i < c then
			t[i] = t[c]
		end
		t[c] = nil
	end
	return t
end

-- ����һ��ȫ��Э������
coroutines = array_create()

-- ����һ��Э��. �������ں���. ��������ִ��( ���ӳ�ִ�е�Ч�� )
go = function(f, ...)
	local args = {...}
	local co = nil
	if #args == 0 then
		co = coroutine_create(f)
	else
		co = coroutine_create(function() f(table_unpack(args)) end)
	end
	coroutines.add(co)
	return co
end

-- Э��˯�� n ֡
sleep = function(n)
	for _ = 1, n do
		yield()
	end
end

-- Э��˯�� n ��
sleeps = function(n)
	local s = os.clock() + n
	while s > os.clock() do
		yield()
	end
end

-- Ū��ȫ�ֱ����洢ÿ֡���� cocos �� delta ֵ
delta = 0

-- ע��֡�ص�, ÿ֡����Э��
cc_frameUpdate(function(delta)
	_G.delta = delta
	local t = coroutines
	if #t == 0 then return end
	local re = resume
	local cs = coroutine_status
	for i = #t, 1, -1 do
		local c = t[i]
		local ok, msg = re(c)
		if cs(c) == "dead" then
			t.remove(i)
		end
	end
end)

-- ����һ��Э��
go(function()
	while true do
		local x, y = spr:getPosition()
		spr:setPosition( x + delta * 20, y )    -- change x
		yield()
	end
end)

print("end")
