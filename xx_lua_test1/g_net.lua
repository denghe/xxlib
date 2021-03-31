require('g_coros')
require('g_objmgr')

-- 全局网络客户端
gNet = NewUvClient()
-- 公用序列化容器
gBB = NewXxData()
-- 公用序列化管理器
gOM = ObjMgr.Create()

-- 网络协程执行状态
NetStates = {
	unknown = "unknown",
	resolve = "resolve",
	dial = "dial",
	wait_0_open = "wait_0_open",
	service_0_ready = "service_0_ready"
}

-- 网络协程运行交互容器
gNetStates = {
	-- 可通知 网络协程 退出
	running = true,
	-- 可检测 网络协程 是否已退出
	closed = false,
	-- 可判断 网络协程 执行到哪步了
	state = NetStates.unknown
}

-- 每帧来一发 gNet Update
gUpdates_Set("gNet", function() gNet:Update() end)







-- 推送的多播处理函数集 key: proto, val: { name + func(serial, pkg)... }
gNetHandlers = {}

-- 注册网络包处理函数
gNetHandlers_Register = function(pkgProto, key, func)
	local t = gNetHandlers[pkgProto]
	if t == nil then
		t = {}
		gNetHandlers[pkgProto] = t
	end
	if t[key] ~= nil then
		print("warning: gNetEvents_Register: pkgProto = "..pkgProto.typeName..", key = "..key.." exists.")
	else
		t[key] = func
	end
end

-- 反注册网络包处理函数
gNetHandlers_Unregister = function(pkgProto, key)
	local t = gNetHandlers[pkgProto]
	if t == nil then
		print("warning: gNetHandlers_Unregister: pkgProto = "..pkgProto.typeName.." can't find any handler!!")
	else
		if t[key] == nil then
			print("warning: gNetHandlers_Unregister: pkgProto = "..pkgProto.typeName..", key = "..key.." does not exists!!")
		else
			t[key] = nil
		end
	end
end

-- service 分组 id 存放点. key: PKG 前缀( 例如 "PKG_Slots_Client_", "PKG_Golden88_"  ), value: serviceId
gNetServiceIdMappings = {}
-- 注册 service 分组 id( 类似收到 进入游戏包 时，需要调用 )
gNet_RegisterServiceMappings = function(pkgPrefix, serviceId)
	gNetServiceIdMappings[pkgPrefix] = serviceId
end

-- 反注册 service 分组 id( 类似收到 离开包 时, 需要调用 )
gNet_UnegisterServiceMappings = function(pkgPrefix)
	gNetServiceIdMappings[pkgPrefix] = nil
end

-- 通过 pkg 猜 serviceId 并返回
gNet_GetServiceId = function(pkg)
	local pkgName = getmetatable(pkg).typeName
	local startIndex = -1
	-- todo: 按 pkgPrefix 由长到短排序 避免 歧义 ??
	for key, value in pairs(gNetServiceIdMappings) do
		startIndex = string.find(pkgName, key)
		if startIndex == 1 then
			return value
		end
	end
	return 0;
end

-- 设置某个 serviceId 的数据走 cpp 那边的 TryGetCppPackage
gNet_SetCppServiceId = function(serviceId)
	gNet:SetCppServiceId(serviceId)
end

-- 全局自增序号发生变量
gNetAutoSerialId = 0

-- 产生一个序号并返回 值, string
gNet_GenerateSerial = function()
	gNetAutoSerialId = gNetAutoSerialId + 1
	return gNetAutoSerialId, tostring(gNetAutoSerialId)
end


-- 全局 序号 回调映射. key: toString(serial)   value: callback_func( pkg )
gNetSerialCallbacks = {}

-- 断线时触发所有回调(nil)
gNet_ClearSerialCallbacks = function()
	for _, cb in pairs(gNetSerialCallbacks) do
		cb(nil)
	end
	gNetSerialCallbacks = {}
end




-- 将 bb 转为 pkg 并返回 pkg. 失败返回 nil
gReadRoot = function(bb)
	if bb ~= nil then
		local success, pkg = gOM:ReadFrom(bb)
		if success then
			return pkg
		--else
			--print(pkg)
		end
	end
end

-- 将 pkg 填充到 bb 并返回 bb
gWriteRoot = function(bb, pkg)
	bb:Clear()
	gOM:WriteTo(bb, pkg)
	return bb
end


-- 发送原型:
-- gNet:SendTo( serviceId, serial, gWriteRoot(gBB, pkg) );


-- 发应答
gNet_SendResponse = function(pkg, serial)
	-- 计算出目标服务id
	local serviceId = gNet_GetServiceId(pkg)
	-- 如果目标服务id未 open 则直接短路返回 nil
	if not gNet:IsOpened(serviceId) then
		print("gNet:IsOpened is failed when gNet_SendResponse", serverId)
		return nil
	end
	-- 发送并返回 int
	return gNet:SendTo( serviceId, serial, gWriteRoot(gBB, pkg) );
end

-- 发送推送
gNet_SendPush = function(pkg)
	return gNet_SendResponse(pkg, 0)
end

-- 发请求. 如果 cb 为 nil 则表示 直接返回收到 response 的数据. 否则返回 SendTo 的返回值 int
gNet_SendRequest = function(pkg, cb, timeoutMS)
	-- 如果已断线就直接短路返回 nil
	if not gNet:Alive() then
		print("gNet:Alive is false when SendRequest")
		if pkg ~= nil then
			dump(pkg)
		end
		return nil
	end
	-- 计算出目标服务id
	local serviceId = gNet_GetServiceId(pkg)
	-- 如果目标服务id未 open 则直接短路返回 nil
	if not gNet:IsOpened(serviceId) then
		print("gNet:IsOpened is failed when SendRequest", serviceId)
		return nil
	end
	-- 参数检查. 如果缺超时时长就补上
	if timeoutMS == nil then
		timeoutMS = 10000
	end
	-- 计算出 serial
	local serial, serialStr = gNet_GenerateSerial()
	-- 发送
	local rtv = gNet:SendTo(serviceId, 0 - serial, gWriteRoot(gBB, pkg));
	-- 发送失败? 立刻返回 rtv
	if rtv ~= 0 then
		print("SendTo failed when SendRequest", serviceId)
		if pkg ~= nil then
			dump(pkg)
		end
		return nil
	end
	-- 回调模式
	if cb ~= nil then
		-- 注册传入的回调函数
		gNetSerialCallbacks[serialStr] = cb
		go(function()
			SleepSecsByClock(timeoutMS)
			local funCall = gNetSerialCallbacks[serialStr]
			if funCall ~= nil then
				funCall(nil)
				ggNetSerialCallbacks[serialStr] = nil
			end
		end)
		-- 立刻返回 SendTo 的结果
		return rtv
	-- return 模式
	else
		-- 用一个引用容器来装载返回值 避免某些版本 lua 出问题
		local t = { [1] = null }
		-- 注册等翻转变量的回调
		gNetSerialCallbacks[serialStr] = function(pkg)
			-- 翻转
			t[1] = pkg
		end
		local now = NowSteadyEpochMS()
		while t[1] == null do
			-- 超时检查
			if NowSteadyEpochMS() - now > timeoutMS then
				-- 反注册: 不等了
				gNetSerialCallbacks[serialStr] = nil
				print("recving package is overtime, call gNet:Disconnect()")
				if pkg ~= nil then
					dump(pkg)
				end
				return nil
			end
			coroutine.yield()
		end
		-- 值应该是 pkg
		return t[1]
	end
end

-- 事件分发. 起个独立协程, 一直执行
go(function()
	local yield = coroutine.yield
::LabBegin::
	yield()
	-- 如果 gNet 未就绪就退出
	if gNet == nil or not gNet:Alive() then
		-- 触发所有回调并清空
		gNet_ClearSerialCallbacks()
		goto LabBegin
	end
	-- 试着 pop 出一条
	local serverId, serial, bb = gNet:TryGetPackage()
	-- 如果没有取出消息就退出
	if serverId == nil then
		goto LabBegin
	end
	-- 解包( 解不出则 pkg 值为 nil )
	local pkg = gReadRoot(bb)
	if pkg == nil then
		-- todo: log??
		goto LabBegin
	end
	-- 收到推送或请求: 去 gNetHandlers 找函数
	if serial <= 0 then
		-- 反转
		serial = -serial
		-- 根据 pkg 的原型定位到一组回调
		local cbs = gNetHandlers[getmetatable(pkg)]
		if cbs ~= nil then
			-- 多播调用
			for name, cb in pairs(cbs) do
				if cb ~= nil then
					cb(pkg, serial)
				end
			end
		else
			if getmetatable(pkg) ~= PKG_Generic_Pong then
				print("NetHandle have no package:",getmetatable(pkg).typeName)
			end
		end
	else
		-- 序列号转为 string 类型
		local serialStr = tostring(serial)
		-- 取出相应的回调
		local cb = gNetSerialCallbacks[serialStr]
		gNetSerialCallbacks[serialStr] = nil
		-- 调用
		if cb ~= nil then
			cb(pkg)
		end
	end
	goto LabBegin
end)
