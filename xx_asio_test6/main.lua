require('pkg')
gServerId_Lobby = 0																		-- ����������� id
-- ... more server id here?
gNet:SetDomainPort("127.0.0.1", 54000)													-- ���� gateway domain & port

-- �����߼�: ���� gateway �� �������� lobby ��Ϣ
go(function()
::LabBegin::
	gNet_Reset()																		-- ��������һ��
	SleepSecs(0.5)																		-- �� ���� Э�� ����һ����Ӧʱ��
	print("gNet_Reset()")

	if not gNet_Dial() then goto LabBegin end											-- ��������������. ʧ�ܾ�����
	print("gNet_Dial() == true")

	if not gNet_WaitOpens(gServerId_Lobby) then goto LabBegin end						-- �� open lobby server. ���߻�ʱ: ����
	print("gNet_WaitOpen(gServerId_Lobby) == true")

::LabProcess::
	local serial, pkg = gNet_TryPop(gServerId_Lobby)									-- �� pop һ�� ���� lobby server �� push / request ��Ϣ
	if pkg == nil then																	-- û��
		yield()																			-- sleep һ��
		if not gNet:Alive() then goto LabBegin end										-- �ѶϿ�������
		goto LabProcess																	-- ������
	end

	print("serial = ", serial, " pkg = ")
	DumpPackage(pkg)
	
	goto LabProcess																		-- �������� ��һ����Ϣ
end)

-- �����߼�: �� lobby open �󲻶Ϸ� ping
go(function()
	-- ����һ�³��ú���
	local SendRequest = function(pkg) return gNet_SendRequest(gServerId_Lobby, pkg) end	-- ���㷢�͵� lobby

	-- Ԥ����һЩ���ð��ṹ
	local ping = Ping.Create()

::LabBegin::
	yield()
	if not gNet_WaitOpens(gServerId_Lobby) then goto LabBegin end

	local ms = NowEpochMS()
	local r = SendRequest(ping)															-- ���͵� lobby ���ȴ����
	if r == nil then goto LabBegin end													-- ��ʱ? �Ͽ�? ����
	print( "ping = ", NowEpochMS() - ms )

	SleepSecs( 3 )																		-- ��� ? ��
	goto LabBegin																		-- ����
end)
