require('pkg')
gServerId_Lobby = 0																		-- 定义大厅服务 id
-- ... more server id here?
gNet:SetDomainPort("127.0.0.1", 54000)													-- 设置 gateway domain & port

-- 主线逻辑: 连接 gateway 并 处理所有 lobby 消息
go(function()
::LabBegin::
	gNet_Reset()																		-- 无脑重置一把
	SleepSecs(0.5)																		-- 给 各种 协程 创造一个反应时间
	print("gNet_Reset()")

	if not gNet_Dial() then goto LabBegin end											-- 域名解析并拨号. 失败就重来
	print("gNet_Dial() == true")

	if not gNet_WaitOpens(gServerId_Lobby) then goto LabBegin end						-- 等 open lobby server. 断线或超时: 重连
	print("gNet_WaitOpen(gServerId_Lobby) == true")

::LabProcess::
	local serial, pkg = gNet_TryPop(gServerId_Lobby)									-- 试 pop 一条 属于 lobby server 的 push / request 消息
	if pkg == nil then																	-- 没有
		yield()																			-- sleep 一次
		if not gNet:Alive() then goto LabBegin end										-- 已断开：重连
		goto LabProcess																	-- 继续等
	end

	print("serial = ", serial, " pkg = ")
	DumpPackage(pkg)
	
	goto LabProcess																		-- 继续处理 下一条消息
end)

-- 并行逻辑: 等 lobby open 后不断发 ping
go(function()
	-- 包裹一下常用函数
	local SendRequest = function(pkg) return gNet_SendRequest(gServerId_Lobby, pkg) end	-- 定点发送到 lobby

	-- 预创建一些常用包结构
	local ping = Ping.Create()

::LabBegin::
	yield()
	if not gNet_WaitOpens(gServerId_Lobby) then goto LabBegin end

	local ms = NowEpochMS()
	local r = SendRequest(ping)															-- 发送到 lobby 并等待结果
	if r == nil then goto LabBegin end													-- 超时? 断开? 重来
	print( "ping = ", NowEpochMS() - ms )

	SleepSecs( 3 )																		-- 间隔 ? 秒
	goto LabBegin																		-- 重来
end)
