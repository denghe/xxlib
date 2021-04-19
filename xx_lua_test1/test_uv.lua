require('g_net')

-- ֡�ص�( C++ call )
function gUpdate()
	gNet:Update()
	coroutine.resume(gNetCoro)
	goexec()
	gNet:Update()
end

-- ����Э��ִ��״̬
NS = {
	unknown = "unknown",
	resolve = "resolve",
	dial = "dial",
	wait_0_open = "wait_0_open",
	service_0_ready = "service_0_ready",
}

-- ȫ�ֻ���
gEnv = {
	-- ģ���������
	host = "192.168.1.53", --"www.baidu.com",
	port = 20000,
	ping_timeout_secs = 10,

	-- ����Э������״̬��������
	-- ��֪ͨ ����Э�� �˳�
	coro_net_running = nil,
	-- �ɼ�� ����Э�� �Ƿ����˳�
	coro_net_closed = nil,
	-- ���ж� ����Э�� ִ�е��Ĳ���
	coro_net_state = nil,
	-- ���������߼�˳��������� ping ֵ( �� ���� �� )
	coro_net_ping = nil,
}

-- ����Э��
function coro_net(printLog)
	--------------------------------------------------------------------------
	-- ��ʼ������״̬
	--------------------------------------------------------------------------
	gEnv.coro_net_running = true
	gEnv.coro_net_closed = false
	gEnv.coro_net_state = NS.unknown
	-- �� ui Э���ܸ�֪�����״̬
	yield()
	yield()
	-- Ԥ�����ֲ�����
	local s, r, f
	local d = gBB
	local pts = gEnv.ping_timeout_secs
	--------------------------------------------------------------------------
	-- ��ʼ��������
	--------------------------------------------------------------------------
::LabResolve::
	if not gEnv.coro_net_running then goto LabExit end
	if printLog then print("resolve") end
	gEnv.coro_net_state = NS.resolve
	-- ͣ�� ����/���� �Ͽ�����
	gNet:Cancel()
	gNet:Disconnect()
	-- ��Ҫ��С˯( ��ֹ����Ƶ�����Լ��������ֶϿ���� callback ��ִ��ʱ�� )
	s = Nows() + 1
	repeat yield() until Nows() > s
	yield()
	yield()
	-- ��ʼ����( Ĭ�� 3 �볬ʱ )
	r = gNet:Resolve(gEnv.host)
	-- ʧ��������
	if r ~= 0 then goto LabResolve end
	-- �ȴ�������ɻ�ʱ
	while gNet:Busy() do
		if not gEnv.coro_net_running then goto LabExit end
		if printLog then print("resolve: busy") end
		yield()
	end
	-- �ý������
	r = gNet:GetIPList()
	-- ����������Ϊ��������
	if #r == 0 then goto LabResolve end
	if printLog then dump(r) end
	--------------------------------------------------------------------------
	-- ��ʼ����
	--------------------------------------------------------------------------
	-- ip list תΪ ip:port ��ӵ� gNet
	gNet:ClearAddresses()
	for _, v in ipairs(r) do
		gNet:AddAddress(v, gEnv.port)
	end
	gEnv.coro_net_state = NS.dial
	if printLog then print("dial") end
	-- ��ʼ����( Ĭ�� 5 �볬ʱ )
	r = gNet:Dial()
	-- ʧ��������
	if r ~= 0 then goto LabResolve end
	-- �ȴ�������ɻ�ʱ
	while gNet:Busy() do
		if not gEnv.coro_net_running then goto LabExit end
		if printLog then print("dial: busy") end
		yield()
	end
	-- �������״̬
	if not gNet:Alive() then
		if printLog then print("dial: timeout") end
		goto LabResolve
	end
	-- ������ɶЭ��
	print("gNet:IsKcp() = ", gNet:IsKcp())
	--------------------------------------------------------------------------
	-- �� 0 �ŷ��� open
	--------------------------------------------------------------------------
	gEnv.coro_net_state = NS.wait_0_open
	if printLog then print("wait_0_open") end
    -- 5 �볬ʱ �ȴ�
    s = Nows() + 5
    repeat
		if not gEnv.coro_net_running then goto LabExit end
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
	gEnv.coro_net_state = NS.service_0_ready
	if printLog then print("service_0_ready") end
	f = false		-- �ѷ��ͱ��
	s = Nows()		-- �ϸ� echo ������ʱ��
	while true do
		yield()
		if not gEnv.coro_net_running then goto LabExit end
		-- �������, ���²���
		if not gNet:Alive() then
			if printLog then print("service_0_ready: peer disconnected.") end
			goto LabResolve
		end
		-- ��ǰʱ��
		local ns = Nows()
		-- �ѷ��� echo
		if f then
			-- ������յ� echo ��������, �� ��ping ���ķ��ͱ��Ϊ δ����
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
			-- ����һ��ʱ��û���յ��κλذ�, �����������ز�
			elseif s + pts < ns then
				if printLog then print("service_0_ready: timeout disconnect. no echo receive.") end
				goto LabResolve
			end
		else
			-- δ���� echo, cd ʱ�䵽�˾� ���� ��ǰ����ֵ
			if s < ns then
				d:Clear()
				d:Wd(ns)
				gNet:SendEcho(d)
				f = true
				s = ns + 5			-- ��Լÿ 5 ����һ��
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

-- ����״̬����Э��
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



-- �����շ��������ļ�
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

-- ��Э��
function coro_main()
	-- ��������״̬����Э��
	go_("coro_net_monitor", coro_net_monitor)

	-- ��������Э��( false: ����ӡ��־ )
	go_("coro_net", coro_net, false)

::LabBegin::
	-- �ȴ� 0 �ŷ��� ready
	while true do
		yield()
		if gEnv.coro_net_state == NS.service_0_ready then
			break
		end
	end

	-- todo: int64 ����

	-- ���� ��¼ ��
	local p = PKG_Client_Login_AuthByUsername.Create()
	p.account_name = "acc1"
	p.username = "un1"
	-- ����
	dump(p)
	local r = gNet_SendRequest(p)
	if r == nil then goto LabBegin end
	dump(r)

	while true do
		yield()
		if not gNet:Alive() then goto LabBegin end
	end
end

-- ������Э��
go_("coro_net", coro_main)

-- ���Ӧ���������
print("test UvClient")

