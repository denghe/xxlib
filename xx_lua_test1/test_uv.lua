print("test UvClient")
require('g_coros')

-- ���ߺ���
dump = function(t)
	for k, v in pairs(t) do
		print(k, v)
	end
end

-- ȫ������ͻ���
gNet = NewUvClient()

-- ÿ֡��һ�� gNet Update
gUpdates_Set("gNet", function() gNet:Update() end)





-- ȫ�ֻ���
g_env = {
	-- ģ���������
	host = "192.168.1.196", --"www.baidu.com",
	port = 20000,

	-- Э�̼�ͨ�ű���

	-- ��֪ͨ net Э�� �˳�
	coro_net_run = true,
	-- �ɼ�� net Э�� �Ƿ����˳�
	coro_net_closed = false,
	-- ���ж� net Э�� ִ�е��Ĳ���
	coro_net_state = gNetStates.unknown
}

-- ����Э��
function coro_net(printLog)
	--------------------------------------------------------------------------
	-- ��ʼ��������
	--------------------------------------------------------------------------
::LabResolve::
	if not g_env.coro_net_run then return end
	if printLog then print("resolve") end
	g_env.coro_net_state = gNetStates.resolve
	-- ͣ�� ����/���� �Ͽ�����
	gNet:Cancel()
	gNet:Disconnect()
	-- ��Ҫ��С˯( ��ֹ����Ƶ�����Լ��������ֶϿ���� callback ��ִ��ʱ�� )
	local s = Nows() + 1
	repeat yield() until Nows() > s
	-- ��ʼ����( Ĭ�� 3 �볬ʱ )
	local r = gNet:Resolve(g_env.host)
	-- ʧ��������
	if r ~= 0 then goto LabResolve end
	-- �ȴ�������ɻ�ʱ
	while gNet:Busy() do
		if not g_env.coro_net_run then return end
		if printLog then print("resolve: busy") end
		yield()
	end
	-- �ý������
	local ips = gNet:GetIPList()
	-- ����������Ϊ��������
	if #ips == 0 then goto LabResolve end
	if printLog then dump(ips) end
	--------------------------------------------------------------------------
	-- ��ʼ����
	--------------------------------------------------------------------------
	-- ips תΪ ip:port ��ӵ� gNet
	gNet:ClearAddresses()
	for _, v in ipairs(ips) do
		gNet:AddAddress(v, g_env.port)
	end
	g_env.coro_net_state = gNetStates.dial
	if printLog then print("dial") end
	-- ��ʼ����( Ĭ�� 5 �볬ʱ )
	r = gNet:Dial()
	-- ʧ��������
	if r ~= 0 then goto LabResolve end
	-- �ȴ�������ɻ�ʱ
	while gNet:Busy() do
		if not g_env.coro_net_run then return end
		if printLog then print("dial: busy") end
		yield()
	end
	-- �������״̬
	if not gNet:Alive() then
		if printLog then print("dial: timeout") end
		goto LabResolve
	end
	--------------------------------------------------------------------------
	-- �� 0 �ŷ��� open
	--------------------------------------------------------------------------
	g_env.coro_net_state = gNetStates.wait_0_open
	if printLog then print("wait_0_open") end
    -- 5 �볬ʱ �ȴ�
    s = Nows() + 5
    repeat
		if not g_env.coro_net_run then return end
        yield()
        -- �������, ���²���
		if not gNet:Alive() then
			if printLog then print("wait 0 open: peer disconnected.") end
			goto LabResolve
		end
        -- �����⵽ 0 �ŷ����� open ������ѭ��
        if gNet:IsOpened(0) then
			if printLog then print("wait 0 open: success") end
			goto LabAfterWait
		end
    until Nows() > s
	-- �� 0 �ŷ��� open ��ʱ: ����
    if printLog then print("wait 0 open: timeout") end
    goto LabResolve
	-- 0 �ŷ����� open
::LabAfterWait::
	g_env.coro_net_state = gNetStates.keepalive
	if printLog then print("keepalive") end
	--------------------------------------------------------------------------
	-- �������� �����հ�
	--------------------------------------------------------------------------
::LabKeepAlive::
	yield()
	-- ��� gNet δ�������˳�
	if gNet == nil or not gNet_Alive() then
		-- �������лص������
		gNet_ClearSerialCallbacks()
		goto LabKeepAlive
	end
	-- ���� pop ��һ��
	local serverId, serial, bb = gNet:TryGetPackage()
	-- ���û��ȡ����Ϣ���˳�
	if serverId == nil then
		goto LabKeepAlive
	end
	-- ���( �ⲻ���� pkg ֵΪ nil )
	local pkg = gReadRoot(bb)
	if pkg == nil then
		-- todo: log??
		goto LabKeepAlive
	end
	-- �յ����ͻ�����: ȥ gNetHandlers �Һ���
	if serial <= 0 then
		-- ��ת
		serial = -serial
		-- ���� pkg ��ԭ�Ͷ�λ��һ��ص�
		local cbs = gNetHandlers[pkg.__proto]
		if cbs ~= nil then
			-- �ಥ����
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
		-- ���к�תΪ string ����
		local serialStr = tostring(serial)
		-- ȡ����Ӧ�Ļص�
		local cb = gNetSerialCallbacks[serialStr]
		gNetSerialCallbacks[serialStr] = nil
		-- ����
		if cb ~= nil then
			cb(pkg)
		end
	end
	goto LabKeepAlive
end

-- UIЭ��: ��ӡ����Э��״̬
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

-- ��������Э��( false: ����ӡ��־ )
go_("net", coro_net, false)

-- ���� UI Э��
go_("ui", coro_ui)
