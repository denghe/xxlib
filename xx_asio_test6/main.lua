go(function()
	gNet:SetDomainPort("127.0.0.1", 54000)												-- ���� gateway domain & port
::LabBegin::
	gNet:Reset()
	SleepSecs(0.5)
	print("::LabBegin::")

	if not gNet_Dial() then																-- ��������������. ʧ�ܾ�����
		goto LabBegin
	end
	print("connected")

	local timeout = NowSteadyEpochMS() + 15000											-- �� open(0) 15��. 
	repeat
		yield()
		if not gNet:Alive() then goto LabBegin end										-- ����: ����
		if gNet:IsOpened(0) then break end												-- �ȵ�: break
	until ( NowSteadyEpochMS() > timeout )

	print("serverId 0 opened")

	while gNet:Alive() do
		print( #gNetRecvs )
	end
end)
