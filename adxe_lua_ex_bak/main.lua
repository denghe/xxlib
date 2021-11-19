-- 令 lua 在 release 模式下也能输出
print = release_print

-- 创建初始场景并运行
local scene = cc_Scene_create()
cc_runWithScene(scene)

-- 创建一个精灵
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

-- 令下列常见调用变得更直接
table_unpack = table.unpack
coroutine_create = coroutine.create
coroutine_status = coroutine.status
resume = coroutine.resume
yield = coroutine.yield

-- 简单的数组结构, 主用于倒序访问并交换删除 if #t > 0 then for i = #t, 1, -1 do ...... end end
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

-- 创建一个全局协程容器
coroutines = array_create()

-- 创建一个协程. 参数跟在后面. 不会立刻执行( 有延迟执行的效果 )
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

-- 协程睡眠 n 帧
sleep = function(n)
	for _ = 1, n do
		yield()
	end
end

-- 协程睡眠 n 秒
sleeps = function(n)
	local s = os.clock() + n
	while s > os.clock() do
		yield()
	end
end

-- 弄个全局变量存储每帧来自 cocos 的 delta 值
delta = 0

-- 注册帧回调, 每帧驱动协程
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

-- 启动一个协程
go(function()
	while true do
		local x, y = spr:getPosition()
		spr:setPosition( x + delta * 20, y )    -- change x
		yield()
	end
end)

print("end")
