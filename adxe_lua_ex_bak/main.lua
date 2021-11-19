-- set lua can print in Release mode
print = release_print

-- get os information
-- OS_WINDOWS = 0, OS_LINUX = 1, OS_MAC = 2, OS_ANDROID = 3, OS_IPHONE = 4, OS_IPAD = 5
targetPlatform = cc_getTargetPlatform()
print("OS = ", targetPlatform)

-- get screen safe area data
originX, originY, winWidth, winHeight = cc_getSafeAreaRect()
print("safe area = ", originX, originY, winWidth, winHeight)

-- calculate center pos
centerX = (originX + winWidth) / 2
centerY = (originY + winHeight) / 2
print("center pos = ", centerX, centerY)

-- init scene & run it
local scene = cc_Scene_create()
cc_runWithScene(scene)

-- create a sprite into scene
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

-- functions cache
table_unpack = table.unpack
coroutine_create = coroutine.create
coroutine_status = coroutine.status
resume = coroutine.resume
yield = coroutine.yield
os_clock = os.clock
math_random = math.random
math_cos = math.cos
math_sin = math.sin

-- simple array
array_create = function()
	local t = {}
	t.add = function(o)
		t[#t + 1] = o
	end
	-- swap erase
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

-- global coroutine container
coroutines = array_create()

-- create a coroutine. delay run
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

-- yield sleep n times
sleep = function(n)
	for _ = 1, n do
		yield()
	end
end

-- yield sleep n seconds
sleeps = function(n)
	local s = os_clock() + n
	while s > os_clock() do
		yield()
	end
end

-- store cocos delta value
delta = 0

-- register frame callback, run coroutines
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

--[[
-- start a coroutine. move spr every frame
go(function()
	while true do
		local x, y = spr:getPosition()
		spr:setPosition( x + delta * 20, y )    -- change x
		yield()
	end
end)

-- start a coroutine. every 2 seconds print coroutines count
go(function()
	local t = coroutines
	while true do
		print("#coroutines = ", #t)
		sleeps(2)
	end
end)
]]

-- function for easy make sprite
local creSpr = function(x, y)
	local s = cc_Sprite_create("HelloWorld.png")
	s:setPosition(x, y)
	scene:addChild(s)
	return s
end

--[[
-- start a coroutine. every frame create a sprite at screen center & random move 10 seconds
-- very bad performance. 1xx coroutine drop fps to 17
go(function()
	while true do
		go(function()
			-- start pos: center screen
			local x = centerX
			local y = centerY
			-- random angle
			local a = math_random() * 3.1416 * 2
			-- set speed
			local spd = 10
			-- calculate frame increase value
			local dx = math_cos(a) * spd
			local dy = math_sin(a) * spd
			-- calculate end time
			local et = os_clock() + 10
			-- create sprite
			local s = creSpr(x, y)
			repeat
				x = x + dx
				y = y + dy
				s:setPosition(x, y)
				yield()
			until( os_clock() > et )
			-- kill sprite
			s:removeFromParent()
		end)
		yield()
	end
end)
]]

-- start a coroutine. every frame create a sprite at screen center & random move 100 seconds
-- very good performance. 20000 updates fps = 60
go(function()
	local updates = array_create()
	-- start a coroutine. every 1 seconds print #updates
	go(function()
		while true do
			print("#updates = ", #updates)
			sleeps(1)
		end
	end)
	while true do
		-- every frame create 10 sprites
		for i = 1, 10 do
			-- start pos: center screen
			local x = centerX
			local y = centerY
			-- random angle
			local a = math_random() * 3.1416 * 2
			-- set speed
			local spd = 1
			-- calculate frame increase value
			local dx = math_cos(a) * spd
			local dy = math_sin(a) * spd
			-- create sprite
			local s = creSpr(x, y)
			-- calculate end time
			local et = os_clock() + 100

			-- put update func to container. return true mean finished. need remove
			updates.add(function()
				x = x + dx
				y = y + dy
				s:setPosition(x, y)
				if os_clock() > et then
					s:removeFromParent()
					return true
				end
				return false
			end)
		end
		-- call all updates
		for i = #updates, 1, -1 do
			if updates[i]() then
				updates.remove(i)
			end
		end
		--
		yield()
	end
end)

print("main end")
