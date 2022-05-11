-- ��������( ���������ṩ���л�֧�� )( �ڲ������û���һ���ò��� )
ObjMgr = {
    -- ���� ObjMgr ʵ��( ��̬���� )
    Create = function()
        local om = {}
        setmetatable(om, ObjMgr)
        return om
    end
    -- ��¼ typeId �� Ԫ�� ��ӳ��( ��̬���� )
, Register = function(o)
        ObjMgr[o.typeId] = o
    end
    -- ��ں���: ʼ�� d д��һ�� "��"
, WriteTo = function(self, d, o)
        assert(o)
        self.d = d
        self.m = { len = 1, [o] = 1 }
        d:Wvu16(getmetatable(o).typeId)
        o:Write(self)
    end
    -- ��ں���: ��ʼ�� d ����һ�� "��" ������ r, o    ( r == 0 ��ʾ�ɹ� )
, ReadFrom = function(self, d)
        self.d = d
        self.m = {}
        return self:ReadFirst()
    end
    -- �ڲ�����: �� d д��һ�� "��". ��ʽ: idx + typeId + content
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
    -- �ڲ�����: �� d ����һ�� "��" ������ r, o    ( r == 0 ��ʾ�ɹ� )
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

-- ��ӡ������
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

local gCoros = {}																				-- ȫ��Э�̳�( ���� )
yield = coroutine.yield																			-- Ϊ�˱���ʹ��

gInt64IsUserdata = type (jit) == 'table'														-- �洢һ���ж�����

-- ѹ��һ��Э�̺���. �в����͸��ں���. ���ӳ�ִ�е�Ч��. ����ʱ�� name ��ʾ
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

-- ѹ��һ��Э�̺���. �в����͸��ں���. ���ӳ�ִ�е�Ч��
go = function(func, ...)
	return go_("", func, ...)
end

-- ˯ָ����( ����˯3֡ )( coro )
SleepSecs = function(secs)
	local timeout = NowEpochMS() + secs * 1000
	yield()
	yield()
	yield()
	while timeout > NowEpochMS() do 
		yield()
	end
end

-- �⼸�����ڲ�����, һ���ò���
local gBB = NewXxData()																			-- �������л�����
local gOM = ObjMgr.Create()																		-- �������л�������
local gSerial = 0																				-- ȫ��������ŷ�������
local gNetReqs = {}																				-- SendRequest ʱע���ڴ� serial : null/pkg


-- gNet ȫ������ͻ��� ���ú���:
-- Update()   Reset()     SetDomainPort("xxx.xxx", 123)      SetSecretKey( ??? )    AddCppServerIds( ? ... )
-- Dial()     Busy()      Alive()      IsOpened( ? )     SendTo( serverId, serial, data )     TryPop() -> serverId, serial, data
gNet = NewAsioTcpGatewayClient()

-- ���յ��� Push & Request ���͵İ�. �� serverId ������
local gNetRecvs = {}

-- �ڲ��������� msgs pop һ�����ݷ���
local TryPopFrom = function(msgs)
	if msgs == nil then return end;
	if #msgs == 0 then return end;
	local r = msgs[1]
	table.remove(msgs, 1)
	return r[1], r[2]																			-- serial, pkg
end

-- �Ӱ��� serverId ����Ľ��ն����� �Ե���һ����Ϣ. ��ʽΪ [serverId, ] serial, pkg ��û�оͷ��� nil
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

-- �ȴ�ָ�� serverId open�����ȵ� �� ���� true ( coro )
gNet_WaitOpens = function(...)
	local timeout = NowEpochMS() + 15000														-- 15 ��
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

-- ����( ���������� �����ѡ�� ip ). �ɹ����Ϸ��� true. ��Ҫ�� SetDomainPort ( coro )
gNet_Dial = function()
	if gNet:Busy() then																			-- ���ֲ��÷��������
		print("gNet:Busy() == true when gNet_Dial()")
		return
	end
	gNet:Dial()																					-- ��ʼ����
	repeat yield() until (not gNet:Busy())														-- �ȵ��� busy
	return gNet:Alive()																			-- �����Ƿ�������
end

-- ����ǰ�ü��, ����Ѷ��߾� ��� �� ���� nil
gNet_SendCheck = function(fn, serverId, pkg)
	if not gNet:Alive() then																	-- ��������ѶϿ�
		print("gNet:Alive() == false when ", fn, " to ", serverId)
		DumpPackage(pkg)
		return false
	end
	if not gNet:IsOpened(serverId) then															-- ��� serverId δ open
		print("gNet:IsOpened(",serverId,") == false when ", fn)
		DumpPackage(pkg)
	end
	return true;
end

-- ��Ӧ��. �ɹ����� true
gNet_SendResponse = function(serverId, serial, pkg)
	if not gNet_SendCheck("gNet_SendResponse", serverId, pkg) then return end
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, serial, gBB);															-- serial > 0
	return true
end

-- ��������. �ɹ����� true
gNet_SendPush = function(serverId, pkg)
	if not gNet_SendCheck("gNet_SendPush", serverId, pkg) then return end
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, 0, gBB);																-- serial == 0
	return true
end

-- ������. �����յ��� response ����. ������� nil ˵����ʱ ( coro )
gNet_SendRequest = function(serverId, pkg)
	if not gNet_SendCheck("gNet_SendRequest", serverId, pkg) then return end
	gSerial = gSerial + 1																		-- ���� serial
	local serial = gSerial
	gBB:Clear()
	gOM:WriteTo(gBB, pkg)
	gNet:SendTo(serverId, 0 - serial, gBB);														-- serial < 0
	gNetReqs[serial] = null																		-- ע����Ӧ serial �ķ�ת����
	local timeout = NowEpochMS() + 15000														-- �õ���ʱʱ���
	repeat
		yield()
		if NowEpochMS() > timeout then															-- ��ʱ:
			gNetReqs[serial] = nil																-- ��ע��
			print("SendRequest timeout")
			DumpPackage(pkg)
			return nil
		end
	until (gNetReqs[serial] ~= null)															-- �ȴ����������
	local r = gNetReqs[serial]																	-- ȡ����
	gNetReqs[serial] = nil																		-- ��ע��
	return r
end

-- �������Э�������ַ�. ���� Push & Request ������ gNetRecvs. ���� Response ��ȥ gNetReqs ���� pkg ( �ڲ������û���һ���ò��� )
local gNetCoro = coroutine.create(function() xpcall( function()
::LabBegin::
	yield()
	if not gNet:Alive() then																	-- ��� gNet δ����
		gNetReqs = {}																			-- ���
		goto LabBegin
	end
	local serverId, serial, data = gNet:TryPop()												-- ���� pop ��һ����Ϣ
	if serverId == nil then																		-- û��ȡ��
		goto LabBegin
	end
	local r, pkg = gOM:ReadFrom(data)															-- ���
	if 0 ~= r  then																				-- ���ʧ��( ͨ��Ϊ���� bug )
		print("ReadFrom data failed. r = ", r)
		goto LabBegin
	end
	if serial <= 0 then																			-- �յ����ͻ�����: ������ gNetRecvs
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

-- ÿ֡�� host ����һ��, ִ������ coros
GlobalUpdate = function()
	gNet:Update()
	local cs = coroutine.status
	local cr = coroutine.resume
	cr(gNetCoro)																				-- ����ִ�� ���ַ�
	local t = gCoros
	if #t == 0 then return end
	for i = #t, 1, -1 do																		-- ������ִ��Э��
		local co = t[i]
		if cs(co) == "dead" then																-- ����ɾ��( �ᵼ������ )
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
