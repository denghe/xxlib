print("test UvClient")
require('g_coros')

-- 工具函数
dump = function(t)
	for k, v in pairs(t) do
		print(k, v)
	end
end

-- 全局网络客户端
gNet = NewUvClient()

-- 每帧来一发 gNet Update
gUpdates_Set("gNet", function() gNet:Update() end)





-- 全局环境
g_env = {
	-- 模拟基础配置
	host = "192.168.1.196", --"www.baidu.com",
	port = 20000,

	-- 协程间通信变量

	-- 可通知 net 协程 退出
	coro_net_run = true,
	-- 可检测 net 协程 是否已退出
	coro_net_closed = false,
	-- 可判断 net 协程 执行到哪步了
	coro_net_state = gNetStates.unknown
}

-- 网络协程
function coro_net(printLog)
	--------------------------------------------------------------------------
	-- 开始域名解析
	--------------------------------------------------------------------------
::LabResolve::
	if not g_env.coro_net_run then return end
	if printLog then print("resolve") end
	g_env.coro_net_state = gNetStates.resolve
	-- 停掉 拨号/解析 断开连接
	gNet:Cancel()
	gNet:Disconnect()
	-- 必要的小睡( 防止拨号频繁，以及留出各种断开后的 callback 的执行时机 )
	local s = Nows() + 1
	repeat yield() until Nows() > s
	-- 开始解析( 默认 3 秒超时 )
	local r = gNet:Resolve(g_env.host)
	-- 失败则重试
	if r ~= 0 then goto LabResolve end
	-- 等待解析完成或超时
	while gNet:Busy() do
		if not g_env.coro_net_run then return end
		if printLog then print("resolve: busy") end
		yield()
	end
	-- 拿解析结果
	local ips = gNet:GetIPList()
	-- 如果解析结果为空则重试
	if #ips == 0 then goto LabResolve end
	if printLog then dump(ips) end
	--------------------------------------------------------------------------
	-- 开始拨号
	--------------------------------------------------------------------------
	-- ips 转为 ip:port 添加到 gNet
	gNet:ClearAddresses()
	for _, v in ipairs(ips) do
		gNet:AddAddress(v, g_env.port)
	end
	g_env.coro_net_state = gNetStates.dial
	if printLog then print("dial") end
	-- 开始拨号( 默认 5 秒超时 )
	r = gNet:Dial()
	-- 失败则重试
	if r ~= 0 then goto LabResolve end
	-- 等待拨号完成或超时
	while gNet:Busy() do
		if not g_env.coro_net_run then return end
		if printLog then print("dial: busy") end
		yield()
	end
	-- 检查连接状态
	if not gNet:Alive() then
		if printLog then print("dial: timeout") end
		goto LabResolve
	end
	--------------------------------------------------------------------------
	-- 等 0 号服务 open
	--------------------------------------------------------------------------
	g_env.coro_net_state = gNetStates.wait_0_open
	if printLog then print("wait_0_open") end
    -- 5 秒超时 等待
    s = Nows() + 5
    repeat
		if not g_env.coro_net_run then return end
        yield()
        -- 如果断线, 重新拨号
		if not gNet:Alive() then
			if printLog then print("wait 0 open: peer disconnected.") end
			goto LabResolve
		end
        -- 如果检测到 0 号服务已 open 就跳出循环
        if gNet:IsOpened(0) then
			if printLog then print("wait 0 open: success") end
			goto LabAfterWait
		end
    until Nows() > s
	-- 等 0 号服务 open 超时: 重连
    if printLog then print("wait 0 open: timeout") end
    goto LabResolve
	-- 0 号服务已 open
::LabAfterWait::
	g_env.coro_net_state = gNetStates.keepalive
	if printLog then print("keepalive") end
	--------------------------------------------------------------------------
	-- 保持连接 处理收包
	--------------------------------------------------------------------------
::LabKeepAlive::
	yield()
	-- 如果 gNet 未就绪就退出
	if gNet == nil or not gNet_Alive() then
		-- 触发所有回调并清空
		gNet_ClearSerialCallbacks()
		goto LabKeepAlive
	end
	-- 试着 pop 出一条
	local serverId, serial, bb = gNet:TryGetPackage()
	-- 如果没有取出消息就退出
	if serverId == nil then
		goto LabKeepAlive
	end
	-- 解包( 解不出则 pkg 值为 nil )
	local pkg = gReadRoot(bb)
	if pkg == nil then
		-- todo: log??
		goto LabKeepAlive
	end
	-- 收到推送或请求: 去 gNetHandlers 找函数
	if serial <= 0 then
		-- 反转
		serial = -serial
		-- 根据 pkg 的原型定位到一组回调
		local cbs = gNetHandlers[pkg.__proto]
		if cbs ~= nil then
			-- 多播调用
			for name, cb in pairs(cbs) do
				if cb ~= nil then
					cb(pkg, serial)
				end
			end
		else
			if pkg.__proto ~= PKG_Generic_Pong then
				print("NetHandle have no package:",pkg.__proto.typeName)
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
	goto LabKeepAlive
end

-- UI协程: 打印网络协程状态
function coro_ui()
	local ls = g_env.coro_net_state
	print("g_env.coro_net_state = "..ls)
	while true do
		if ls ~= g_env.coro_net_state then
			ls = g_env.coro_net_state
			print("g_env.coro_net_state = "..ls)
		end
		yield()
	end
end

-- 启动网络协程( false: 不打印日志 )
go_("net", coro_net, false)

-- 启动 UI 协程
go_("ui", coro_ui)
