-- 包管理器( 对生成物提供序列化支撑 )( 内部对象，用户层一般用不到 )
ObjMgr = {
    -- 创建 ObjMgr 实例( 静态函数 )
    Create = function()
        local om = {}
        setmetatable(om, ObjMgr)
        return om
    end
    -- 记录 typeId 到 元表 的映射( 静态函数 )
, Register = function(o)
        ObjMgr[o.typeId] = o
    end
    -- 入口函数: 始向 d 写入一个 "类"
, WriteTo = function(self, d, o)
        assert(o)
        self.d = d
        self.m = { len = 1, [o] = 1 }
        d:Wvu16(getmetatable(o).typeId)
        o:Write(self)
    end
    -- 入口函数: 开始从 d 读出一个 "类" 并返回 r, o    ( r == 0 表示成功 )
, ReadFrom = function(self, d)
        self.d = d
        self.m = {}
        return self:ReadFirst()
    end
    -- 内部函数: 向 d 写入一个 "类". 格式: idx + typeId + content
, Write = function(self, o)
        local d = self.d
        if o == null or o == nil then
            d:Wu8(0)
        else
            local m = self.m
            local n = m[o]
            if n == nil then
                n = m.len + 1
                m.len = n
                m[o] = n
                d:Wvu32(n)
                d:Wvu16(getmetatable(o).typeId)
                o:Write(self)
            else
                d:Wvu32(n)
            end
        end
    end
    -- 内部函数: 从 d 读出一个 "类" 并返回 r, o    ( r == 0 表示成功 )
, ReadFirst = function(self)
        local d = self.d
        local m = self.m
        local r, typeId = d:Rvu16()
        if r ~= 0 then
            return 53
        end
        if typeId == 0 then
            return 56
        end
        local mt = ObjMgr[typeId]
        if mt == nil then
            return 60
        end
        local v = mt.Create()
        m[1] = v
		r = v:Read(self)
        if r ~= 0 then
            return 66
        end
        return 0, v
    end
, Read = function(self)
        local d = self.d
        local r, n = d:Rvu32()
        if r ~= 0 then
            return r
        end
        if n == 0 then
            return 0, null
        end
        local m = self.m
        local len = #m
		local typeId
        if n == len + 1 then
            r, typeId = d:Rvu16()
            if r ~= 0 then
                return r
            end
            if typeId == 0 then
                return 88
            end
            local mt = ObjMgr[typeId]
            if mt == nil then
                return 92
            end
            local v = mt.Create()
            m[n] = v
            r = v:Read(self)
            if r ~= 0 then
                return r
            end
            return 0, v
        else
            if n > len then
                return 103
            end
            return 0, m[n]
        end
    end
}
ObjMgr.__index = ObjMgr

-- 打印包内容
DumpPackage = function(t)
	print("===============================================")
	print("|||||||||||||||||||||||||||||||||||||||||||||||")
	local mt = getmetatable(t)
	if mt ~= nil then
		print("typeName:", mt.typeName)
	end
	for k, v in pairs(t) do
		print(k, v)
	end
	print("|||||||||||||||||||||||||||||||||||||||||||||||")
	print("===============================================")
end

local gCoros = {}																				-- 全局协程池( 乱序 )
yield = coroutine.yield																			-- 为了便于使用

gInt64IsUserdata = type (jit) == 'table'														-- 存储一个判断依据

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果. 报错时带 name 显示
local go_ = function(name, func, ...)
	local f = function(msg) print("coro ".. name .." error: " .. tostring(msg) .. "\n")  end
	local args = {...}
	local p
	if #args == 0 then
		p = function() xpcall(func, f) end
	else
		p = function() xpcall(func, f, table.unpack(args)) end
	end
	local co = coroutine.create(p)
	table.insert(gCoros, co)
	return co
end

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果
go = function(func, ...)
	return go_("", func, ...)
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

-- 这几个是内部变量, 一般用不到
local gBB = NewXxData()																			-- 公用序列化容器
local gOM = ObjMgr.Create()																		-- 公用序列化管理器
local gSerial = 0																				-- 全局自增序号发生变量
local gNetReqs = {}																				-- SendRequest 时注册在此 serial : null/pkg


-- gNet 全局网络客户端 可用函数:
-- Update()   Reset()     SetDomainPort("xxx.xxx", 123)      SetSecretKey( ??? )    AddCppServerIds( ? ... )
-- Dial()     Busy()      Alive()      IsOpened( ? )     SendTo( serverId, serial, data )     TryPop() -> serverId, serial, data
gNet = NewAsioTcpGatewayClient()

-- 已收到的 Push & Request 类型的包. 按 serverId 分组存放
local gNetRecvs = {}

-- 内部函数。从 msgs pop 一条数据返回
local TryPopFrom = function(msgs)
	if msgs == nil then return end;
	if #msgs == 0 then return end;
	local r = msgs[1]
	table.remove(msgs, 1)
	return r[1], r[2]																			-- serial, pkg
end

-- 从按照 serverId 分组的接收队列中 试弹出一条消息. 格式为 [serverId, ] serial, pkg 。没有就返回 nil
gNet_TryPop = function(serverId)
	if serverId == nil then
		for sid, msgs in pairs(gNetRecvs) do
			local serial, pkg = TryPopFrom(msgs)
			if serial ~= nil then
				return sid, serial, pkg
			end
		end
	else
		local msgs = gNetRecvs[serverId]
		return TryPopFrom(msgs)
	end
end

-- 等待指定 serverId open。都等到 则 返回 true ( coro )
gNet_WaitOpens = function(...)
	local timeout = NowEpochMS() + 15000														-- 15 秒
	repeat
		yield()
		if not gNet:Alive() then return end
		local allOpen = true
		for _, id in ipairs({...}) do
			if not gNet:IsOpened(id) then
				allOpen = false
				break
			end
		end
		if allOpen then return true end
	until ( NowEpochMS() > timeout )
end

-- 拨号( 含域名解析 并随机选择 ip ). 成功连上返回 true. 需要先 SetDomainPort ( coro )
gNet_Dial = function()
	if gNet:Busy() then																			-- 出现不该发生的情况
		print("gNet:Busy() == true when gNet_Dial()")
		return
	end
	gNet:Dial()																					-- 开始拨号
	repeat yield() until (not gNet:Busy())														-- 等到不 busy
	return gNet:Alive()																			-- 返回是否已连上
end

-- 发送前置检查, 如果已断线就 输出 并 返回 nil
gNet_SendCheck = function(fn, serverId, pkg)
	if not gNet:Alive() then																	-- 如果网络已断开
		print("gNet:Alive() == false when ", fn, " to ", serverId)
		DumpPackage(pkg)
		return false
	end
	if not gNet:IsOpened(serverId) then															-- 如果 serverId 未 open
		print("gNet:IsOpened(",serverId,") == false when ", fn)
		DumpPackage(pkg)
	end
	return true;
end

-- 发应答. 成功返回 true
gNet_SendResponse = function(serverId, serial, pkg)
	if not gNet_SendCheck("gNet_SendResponse", serverId, pkg) then return end
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, serial, gBB);															-- serial > 0
	return true
end

-- 发送推送. 成功返回 true
gNet_SendPush = function(serverId, pkg)
	if not gNet_SendCheck("gNet_SendPush", serverId, pkg) then return end
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, 0, gBB);																-- serial == 0
	return true
end

-- 发请求. 返回收到的 response 数据. 如果返回 nil 说明超时 ( coro )
gNet_SendRequest = function(serverId, pkg)
	if not gNet_SendCheck("gNet_SendRequest", serverId, pkg) then return end
	gSerial = gSerial + 1																		-- 生成 serial
	local serial = gSerial
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, 0 - serial, gBB);														-- serial < 0
	gNetReqs[serial] = null																		-- 注册相应 serial 的反转变量
	local timeout = NowEpochMS() + 15000														-- 得到超时时间点
	repeat
		yield()
		if NowEpochMS() > timeout then															-- 超时:
			gNetReqs[serial] = nil																-- 反注册
			print("SendRequest timeout")
			DumpPackage(pkg)
			return nil
		end
	until (gNetReqs[serial] ~= null)															-- 等待变量被填充
	local r = gNetReqs[serial]																	-- 取出来
	gNetReqs[serial] = nil																		-- 反注册
	return r
end

-- 起个独立协程做包分发. 遇到 Push & Request 包就塞 gNetRecvs. 遇到 Response 就去 gNetReqs 设置 pkg ( 内部对象，用户层一般用不到 )
local gNetCoro = coroutine.create(function() xpcall( function()
::LabBegin::
	yield()
	if not gNet:Alive() then																	-- 如果 gNet 未就绪
		gNetReqs = {}																			-- 清空
		goto LabBegin
	end
	local serverId, serial, data = gNet:TryPop()												-- 试着 pop 出一条消息
	if serverId == nil then																		-- 没有取到
		goto LabBegin
	end
	local r, pkg = gOM:ReadFrom(data)															-- 解包
	if 0 ~= r  then																				-- 解包失败( 通常为代码 bug )
		print("ReadFrom data failed. r = ", r)
		goto LabBegin
	end
	if serial <= 0 then																			-- 收到推送或请求: 分组塞 gNetRecvs
		local msgs = gNetRecvs[serverId]
		if msgs == nil then
			msgs = {}
			gNetRecvs[serverId] = msgs
		end
		table.insert(msgs, { [1] = -serial, [2] = pkg })
	else
		if gNetReqs[serial] == null then
			gNetReqs[serial] = pkg
		end
	end
	goto LabBegin
end,
function(msg) print("coro gNetCoro error: " .. tostring(msg) .. "\n")  end ) end )

-- 每帧被 host 调用一次, 执行所有 coros
GlobalUpdate = function()
	gNet:Update()
	local cs = coroutine.status
	local cr = coroutine.resume
	cr(gNetCoro)																				-- 先行执行 包分发
	local t = gCoros
	if #t == 0 then return end
	for i = #t, 1, -1 do																		-- 遍历并执行协程
		local co = t[i]
		if cs(co) == "dead" then																-- 交换删除( 会导致乱序 )
			t[i] = t[#t]
			t[#t] = nil
		else
			local ok, msg = cr(co)
			if not ok then
				print("coroutine.resume error:", msg)
			end
		end
	end
	gNet:Update()
end
