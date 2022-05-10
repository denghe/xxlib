go(function()
	gNet:SetDomainPort("127.0.0.1", 54000)												-- 设置 gateway domain & port
::LabBegin::
	gNet:Reset()
	SleepSecs(0.5)
	print("::LabBegin::")

	if not gNet_Dial() then																-- 域名解析并拨号. 失败就重来
		goto LabBegin
	end
	print("connected")

	local timeout = NowSteadyEpochMS() + 15000											-- 等 open(0) 15秒. 
	repeat
		yield()
		if not gNet:Alive() then goto LabBegin end										-- 断线: 重来
		if gNet:IsOpened(0) then break end												-- 等到: break
	until ( NowSteadyEpochMS() > timeout )

	print("serverId 0 opened")

	while gNet:Alive() do
		print( #gNetRecvs )
	end
end)
