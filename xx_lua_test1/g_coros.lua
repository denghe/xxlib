-- 全局协程池( 乱序 )
local gCoros = {}

-- 压入一个经过 coroutine.create + coroutine.resume 传参的 co
gCoros_PushCo = function(co)
	table.insert(gCoros, co)
end

function gCoros_Traceback(msg)
    print("CORO ERROR: " .. tostring(msg) .. "\n")
end

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果. name 在报错的时候体现
go_ = function(name, func, ...)
	local args = {...}, p
	if #args == 0 then
		p = function() xpcall(func, function(msg) print("CORO ".. name .." ERROR: " .. tostring(msg) .. "\n")  end) end
	else
		p = function() xpcall(func, function(msg) print("CORO ".. name .." ERROR: " .. tostring(msg) .. "\n")  end, table.unpack(args)) end
	end
	local co = coroutine.create(p)
	table.insert(gCoros, co)
	return co
end

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果
go = function(func, ...)
	return go_("", func, ...)
end

-- 压入一个协程函数 并立刻 resume 一次. 有参数就跟在后面
gorun  = function(func, ...)
	local co = go(func, ...)
	local ok, msg = coroutine.resume(co)
	if not ok then
		print("coroutine.resume error:", msg)
	end
	return co
end

-- 执行 gCoros 里面的所有协程
gCoros_Exec = function()
	local t = gCoros
	if #t > 0 then
		for i = #t, 1, -1 do
			local co = t[i]
			if coroutine.status(co) == "dead" then
				table.remove(t, i)	-- todo: swap remove?
			else
				local ok, msg = coroutine.resume(co)
				if not ok then
					print("coroutine.resume error:", msg)
				end
			end
		end
	end
end

yield = coroutine.yield



-- 用于注册更新函数. 直接往里面放函数或移除. 每帧将逻辑无序遍历执行.
gUpdates = {}

-- 比自己直接在表上赋值安全点
gUpdates_Set = function(key, func)
	if gUpdates[key] ~= nil then
		print("warning: gUpdates_Set key = "..key.." exists.")
	else
		gUpdates[key] = func
	end
end

gUpdates_Close = function(key)
	if(gUpdates[key] ~= nil)then
		gUpdates[key] = nil
	end
end

-- 执行所有 update 函数
gUpdates_Exec = function()
	local t = gUpdates
	for k, f in pairs(t) do
		f()
	end
end


-- 核心帧回调( 考虑到效率，展开了 gUpdates_Exec & gCoros_Exec )
gFrameCallback = function()
	local cs = coroutine.status
	local cr = coroutine.resume
	local tr = table.remove

	local t = gUpdates
	for k, f in pairs(t) do
		f()
	end

	t = gCoros
	if #t == 0 then return end
	for i = #t, 1, -1 do
		local co = t[i]
		if cs(co) == "dead" then
			tr(t, i)
		else
			local ok, msg = cr(co)
			if not ok then
				print("coroutine.resume error:", msg)
			end
		end
	end
end
