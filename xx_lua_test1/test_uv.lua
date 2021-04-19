require('g_net')

-- 帧回调( C++ call )
function gUpdate()
	gNet:Update()
	coroutine.resume(gNetCoro)
	goexec()
	gNet:Update()
end

-- 网络协程执行状态
NS = {
	unknown = "unknown",
	resolve = "resolve",
	dial = "dial",
	wait_0_open = "wait_0_open",
	service_0_ready = "service_0_ready",
}

-- 全局环境
gEnv = {
	-- 模拟基础配置
	host = "192.168.1.53", --"www.baidu.com",
	port = 20000,
	ping_timeout_secs = 10,

	-- 网络协程运行状态交互变量
	-- 可通知 网络协程 退出
	coro_net_running = nil,
	-- 可检测 网络协程 是否已退出
	coro_net_closed = nil,
	-- 可判断 网络协程 执行到哪步了
	coro_net_state = nil,
	-- 保持连接逻辑顺便算出来的 ping 值( 到 网关 的 )
	coro_net_ping = nil,
}

-- 网络协程
function coro_net(printLog)
	--------------------------------------------------------------------------
	-- 初始化运行状态
	--------------------------------------------------------------------------
	gEnv.coro_net_running = true
	gEnv.coro_net_closed = false
	gEnv.coro_net_state = NS.unknown
	-- 让 ui 协程能感知到这个状态
	yield()
	yield()
	-- 预声明局部变量
	local s, r, f
	local d = gBB
	local pts = gEnv.ping_timeout_secs
	--------------------------------------------------------------------------
	-- 开始域名解析
	--------------------------------------------------------------------------
::LabResolve::
	if not gEnv.coro_net_running then goto LabExit end
	if printLog then print("resolve") end
	gEnv.coro_net_state = NS.resolve
	-- 停掉 拨号/解析 断开连接
	gNet:Cancel()
	gNet:Disconnect()
	-- 必要的小睡( 防止拨号频繁，以及留出各种断开后的 callback 的执行时机 )
	s = Nows() + 1
	repeat yield() until Nows() > s
	yield()
	yield()
	-- 开始解析( 默认 3 秒超时 )
	r = gNet:Resolve(gEnv.host)
	-- 失败则重试
	if r ~= 0 then goto LabResolve end
	-- 等待解析完成或超时
	while gNet:Busy() do
		if not gEnv.coro_net_running then goto LabExit end
		if printLog then print("resolve: busy") end
		yield()
	end
	-- 拿解析结果
	r = gNet:GetIPList()
	-- 如果解析结果为空则重试
	if #r == 0 then goto LabResolve end
	if printLog then dump(r) end
	--------------------------------------------------------------------------
	-- 开始拨号
	--------------------------------------------------------------------------
	-- ip list 转为 ip:port 添加到 gNet
	gNet:ClearAddresses()
	for _, v in ipairs(r) do
		gNet:AddAddress(v, gEnv.port)
	end
	gEnv.coro_net_state = NS.dial
	if printLog then print("dial") end
	-- 开始拨号( 默认 5 秒超时 )
	r = gNet:Dial()
	-- 失败则重试
	if r ~= 0 then goto LabResolve end
	-- 等待拨号完成或超时
	while gNet:Busy() do
		if not gEnv.coro_net_running then goto LabExit end
		if printLog then print("dial: busy") end
		yield()
	end
	-- 检查连接状态
	if not gNet:Alive() then
		if printLog then print("dial: timeout") end
		goto LabResolve
	end
	-- 看看是啥协议
	print("gNet:IsKcp() = ", gNet:IsKcp())
	--------------------------------------------------------------------------
	-- 等 0 号服务 open
	--------------------------------------------------------------------------
	gEnv.coro_net_state = NS.wait_0_open
	if printLog then print("wait_0_open") end
    -- 5 秒超时 等待
    s = Nows() + 5
    repeat
		if not gEnv.coro_net_running then goto LabExit end
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
	gEnv.coro_net_state = NS.service_0_ready
	if printLog then print("service_0_ready") end
	f = false		-- 已发送标记
	s = Nows()		-- 上个 echo 包发送时间
	while true do
		yield()
		if not gEnv.coro_net_running then goto LabExit end
		-- 如果断线, 重新拨号
		if not gNet:Alive() then
			if printLog then print("service_0_ready: peer disconnected.") end
			goto LabResolve
		end
		-- 当前时间
		local ns = Nows()
		-- 已发送 echo
		if f then
			-- 如果有收到 echo 返回数据, 就 算ping 并改发送标记为 未发送
			if #gNetEchos > 0 then
				local v
				r, v = gNetEchos[1]:Rd()
				if r ~= 0 then
					print("read echo data error: r = ", r)
				else
					gEnv.coro_net_ping = ns - v
					gNetEchos = {}
					f = false
				end
			-- 超过一定时长没有收到任何回包, 就主动掐线重拨
			elseif s + pts < ns then
				if printLog then print("service_0_ready: timeout disconnect. no echo receive.") end
				goto LabResolve
			end
		else
			-- 未发送 echo, cd 时间到了就 发送 当前秒数值
			if s < ns then
				d:Clear()
				d:Wd(ns)
				gNet:SendEcho(d)
				f = true
				s = ns + 5			-- 大约每 5 秒来一发
			end
		end
	end
::LabExit::
	gNet:Cancel()
	gNet:Disconnect()
	yield()
	yield()
	gEnv.coro_net_running = false
	gEnv.coro_net_closed = true
	gEnv.coro_net_state = NS.unknown
end

-- 网络状态监视协程
function coro_net_monitor()
	local running = "nil"
	local closed = "nil"
	local state = "nil"
	local ping = "nil"
	local s
	local e = gEnv
	while true do
		s = tostring(e.coro_net_running)
		if running ~= s then
			print("gEnv.coro_net_running = "..s)
			running = s
		end

		s = tostring(e.coro_net_closed)
		if closed ~= s then
			print("gEnv.coro_net_closed = "..s)
			closed = s
		end

		s = tostring(e.coro_net_state)
		if state ~= s then
			print("gEnv.coro_net_state = "..s)
			state = s
		end

		s = tostring(e.coro_net_ping)
		if ping ~= s then
			print("gEnv.coro_net_ping = "..s)
			ping = s
		end

		yield()
	end
end



-- 加载收发包定义文件
require('class_def')
require('client_lobby')
require('client_login')
require('generic')


--local d = NewXxData()
--local t = {244,9,10,2,243,9,0,0,0,0,224,250,3,65,218,166,135,137,170,228,190,57,1,50,0,3,49,50,51,0,2,3,243,9,0,0,0,0,216,250,3,65,170,237,173,247,137,228,190,57,6,54,53,55,53,54,55,0,0,0,0,4,243,9,0,0,0,0,208,250,3,65,216, 195,129,149,137,228,190,57,6,54,53,55,53,54,55,0,0,0,0,5,243,9,0,0,0,0,200,250,3,65,148,196,202,178,136,228,190,57,6,54,53,55,53,54,55,0,0,0,0,6,243,9,0,0,0,0,192,250,3,65,132,197,216,220,247,227,190,57,3,49,50,51,0,0,0,0,7,243,9,0,0,0,0,184,250,3,65,130,211,171,250,246,227,190,57,3,49,50,51,0,0,0,0,8,243,9,0,0,0,0,176,250,3,65,204,176,250,151,246,227,190,57,3,49,50,51,0,0,0,0,9,243,9,0,0,0,0,168,250,3,65,202,214,214,180,245,227,190,57,3,49,50,51,0,0,0,0,10,243,9,0,0,0,0,160,250,3,65,252,187,158,210,244,227,190,57,3,49,50,51,0,0,0,0,11,243,9,0,0,0,0,152,250,3,65,196,238,238,239,243,227,190,57,3,49,50,51,0,0,0,0}
--for _, v in ipairs(t) do
--	d:Wu8(v)
--end
--print(d)
--local r, pkg = gOM:ReadFrom(d)
--print(r, pkg)

-- 主协程
function coro_main()
	-- 启动网络状态监视协程
	go_("coro_net_monitor", coro_net_monitor)

	-- 启动网络协程( false: 不打印日志 )
	go_("coro_net", coro_net, false)

::LabBegin::
	-- 等待 0 号服务 ready
	while true do
		yield()
		if gEnv.coro_net_state == NS.service_0_ready then
			break
		end
	end

	-- todo: int64 兼容

	-- 构造 登录 包
	local p = PKG_Client_Login_AuthByUsername.Create()
	p.account_name = "acc1"
	p.username = "un1"
	-- 发送
	dump(p)
	local r = gNet_SendRequest(p)
	if r == nil then goto LabBegin end
	dump(r)

	while true do
		yield()
		if not gNet:Alive() then goto LabBegin end
	end
end

-- 启动主协程
go_("coro_net", coro_main)

-- 这个应该最先输出
print("test UvClient")

