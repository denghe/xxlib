// test xx_lua_asio_tcp_gateway_client.h 
// 等价于 xx_asio_testC, 属于它的 lua 版

#include <xx_lua_asio_tcp_gateway_client.h>

struct Logic : asio::noncopyable {
	xx::Lua::State L;
	Logic() {
		xx::Lua::Data::Register(L);
		xx::Lua::Asio::Tcp::Gateway::Client::Register(L);
		xx::Lua::SetGlobalCClosure(L, "NowSteadyEpochMS", [](auto L)->int { return xx::Lua::Push(L, (double)xx::NowSteadyEpochMilliseconds()); });
		xx::Lua::DoString(L, R"(

-- 全局协程池( 乱序 )
local gCoros = {}

-- 压入一个协程函数. 有参数就跟在后面. 有延迟执行的效果. 报错时带 name 显示
go_ = function(name, func, ...)
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

-- 核心帧回调. 执行所有 coros
gCorosRun = function()
	local cs = coroutine.status
	local cr = coroutine.resume
	local t = gCoros
	if #t == 0 then return end
	-- print("############## co - count ############### :" .. #t)
	for i = #t, 1, -1 do
		local co = t[i]
		if cs(co) == "dead" then
			t[i] = t[#t]
			t[#t] = nil
		else
			local ok, msg = cr(co)
			if not ok then
				print("coroutine.resume error:", msg)
			end
		end
	end
end

-- 为了便于使用
yield = coroutine.yield

-- 睡指定秒( 保底睡3帧 )( coro )
SleepSecs = function(secs)
	local over = NowSteadyEpochMS() + secs * 1000
	yield()
	yield()
	yield()
	while over > NowSteadyEpochMS() do 
		yield()
	end
end

-- 包管理器( 对生成物提供序列化支撑 )
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
            return r
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
            return r
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
                return 87
            end
            local mt = ObjMgr[typeId]
            if mt == nil then
                return 91
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
                return 102
            end
            return 0, m[n]
        end
    end
}
ObjMgr.__index = ObjMgr

-- 全局网络客户端
gNet = NewAsioTcpGatewayClient()

-- 公用序列化容器
gBB = NewXxData()

-- 公用序列化管理器
gOM = ObjMgr.Create()

-- 全局自增序号发生变量
gSerial = 0

-- 全局 序号 回调映射 for SendRequest
gNetSerialCallbacks = {}

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

-- 发应答
gNet_SendResponse = function(pkg, serial)
	if not gNet:Alive() then																	-- 如果已断线就直接短路返回 nil
		print("gNet:Alive is false when SendResponse")
		if pkg ~= nil then
			DumpPackage(pkg)
		end
		return nil
	end
	
	-- 计算出目标服务id
	local serverId = gNet_GetServiceId(pkg)
	-- 如果目标服务id未 open 则直接短路返回 nil
	if not gNet:IsOpened(serverId) then
		print("gNet:IsOpened is failed when SendResponse", serverId)
		return nil
	end
	-- 发送并返回 int
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo( serverId, serial, gBB );
	return 0
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
			DumpPackage(pkg)
		end
		return nil
	end

	-- 计算出目标服务id
	local serverId = gNet_GetServiceId(pkg)

	-- 如果目标服务id未 open 则直接短路返回 nil
	if not gNet:IsOpened(serverId) then
		print("gNet:IsOpened is failed when SendRequest", serverId)
		return nil
	end

	-- 参数检查. 如果缺超时时长就补上
	if timeoutMS == nil then
		timeoutMS = 10000
	end

	-- 计算 serial
	gSerial = gSerial + 1
	local serial = gSerial

	-- 发送
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, 0 - serial, gBB);

	-- 回调模式
	if cb ~= nil then
		-- 注册传入的回调函数
		gNetSerialCallbacks[serial] = cb
		go(function()
			SleepSecsByClock(timeoutMS)
			local funCall = gNetSerialCallbacks[serial]
			if funCall ~= nil then
				funCall(nil)
				gNetSerialCallbacks[serial] = nil
			end
		end)
		-- 立刻返回 SendTo 的结果
		return rtv
	-- return 模式
	else
		-- 用一个引用容器来装载返回值 避免某些版本 lua 出问题
		local t = { [1] = null }
		-- 注册等翻转变量的回调
		gNetSerialCallbacks[serial] = function(pkg)
			-- 翻转
			t[1] = pkg
		end
		local now = NowSteadyEpochMS()
		while t[1] == null do
			-- 超时检查
			if NowSteadyEpochMS() - now > timeoutMS then
				-- 反注册: 不等了
				gNetSerialCallbacks[serial] = nil
				print("SendRequest timeout")
				if pkg ~= nil then
					DumpPackage(pkg)
				end
				return nil
			end
			coroutine.yield()
		end
		-- 值应该是 pkg
		return t[1]
	end
end

-- 事件分发. 起个独立协程, 一直执行.
gNetCoro = coroutine.create(function() xpcall(
function()
::LabBegin::
	yield()
	if not gNet:Alive() then																	-- 如果 gNet 未就绪
		for _, cb in pairs(gNetSerialCallbacks) do												-- 用 nil 触发所有 Request 等待
			cb(nil)
		end
		gNetSerialCallbacks = {}																-- 清空
		goto LabBegin
	end
	local serverId, serial, data = gNet:TryPop()												-- 试着 pop 出一条消息
	if serverId == nil then																		-- 如果没有取出消息就退出
		goto LabBegin
	elseif serverId == 0xFFFFFFFF then
		table.insert(gNetEchos, data)
		goto LabBegin
	end
	-- 解包( 解不出则 pkg 值为 nil )
	local success, pkg = gOM:ReadFrom(data)

	if 0 ~= success  then
		print("解析失败 ，success:",success)
		-- todo: log??
		goto LabBegin
	end
	-- 收到推送或请求: 去 gNetHandlers 找函数
	if serial <= 0 then
		-- 反转
		serial = -serial
		-- 根据 pkg 的元表定位到一组回调
		local cbs = gNetHandlers[getmetatable(pkg)]
		if cbs ~= nil then
			-- 多播调用
			for _, cb in pairs(cbs) do
				if cb ~= nil then
					cb(pkg, serial)
				end
			end
		end
	else
		-- 序列号转为 string 类型
		local serial = tostring(serial)
		-- 取出相应的回调
		local cb = gNetSerialCallbacks[serial]
		gNetSerialCallbacks[serial] = nil
		-- 调用
		if cb ~= nil then
			cb(pkg)
		end
	end
	goto LabBegin
end,
function(msg) print("coro gNetCoro error: " .. tostring(msg) .. "\n")  end
)
end
)

function GlobalUpdate()
	gCorosRun()
end
)");
		xx::Lua::DoString(L, R"(

)");

	}
	void Update() {
		xx::Lua::CallGlobalFunc(L, "GlobalUpdate");
	}
};

#ifdef _WIN32
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib") 
#endif

int main() {
#ifdef _WIN32
	timeBeginPeriod(1);
#endif

	Logic logic;
	do {
		logic.Update();																			// 每帧来一发
		std::this_thread::sleep_for(16ms);														// 模拟游戏循环延迟
	} while (true);
	xx::CoutN("end.");
	return 0;
}
