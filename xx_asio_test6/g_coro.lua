yield = coroutine.yield																			-- 为了便于使用

local gCoros = {}																				-- 全局协程池( 乱序 )

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果. 报错时带 name 显示
-- 参数传入  func + args...  或  name + func + args...
go = function(...)
	local args = {...}
	local t1 = type(args[1])
	local t2 = type(args[2])
	assert( #args > 0 and ( t1 == 'function' or t2 == 'function') )
	local name
	local func
	if t1 ~= 'function' then
		name = args[1]
		func = args[2]
		table.remove(args, 1)
	else
		func = args[1]
	end
	table.remove(args, 1)
	local ef = function(msg) print("coro ".. name .." error: " .. tostring(msg) .. "\n")  end
	local p
	if #args == 0 then
		p = function() xpcall(func, ef) end
	else
		p = function() xpcall(func, ef, table.unpack(args)) end
	end
	local co = coroutine.create(p)
	table.insert(gCoros, co)
	return co
end

-- 睡指定秒( 保底睡3帧 )( coro )
SleepSecs = function(secs)
	local timeout = NowEpochMS() + secs * 1000
	yield()
	yield()
	yield()
	while timeout > NowEpochMS() do 
		yield()
	end
end

-- 留个口子
gCoro = coroutine.create(function() while true do yield() end end)
gUpdate1 = function() end
gUpdate2 = function() end

-- 每帧被 host 调用一次, 执行所有 coros
GlobalUpdate = function()
	gUpdate1()
	local cr = coroutine.resume
	cr(gCoro)
	local t = gCoros
	local n = #t
	if n == 0 then return end
	for i = n, 1, -1 do																		-- 遍历并执行协程
		if not cr(t[i]) then
			t[i] = t[n]																		-- 交换删除( 会导致乱序 )
			t[n] = nil
			n = n - 1
		end
	end
	gUpdate2()
end
