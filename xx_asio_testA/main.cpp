// lobby + gateway + client -- lobby
#include "xx_asio_codes.h"
#include "pkg.h"

struct GPeer;
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;
};

struct GPeer : xx::PeerCode<GPeer>, xx::PeerTimeoutCode<GPeer>, xx::PeerRequestCode<GPeer, true>, std::enable_shared_from_this<GPeer> {
	Server& server;
	GPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
	{
        ResetTimeout(15s);  // 设置初始的超时时长
    }
	uint32_t gatewayId = 0xFFFFFFFFu;	// gateway 连上来后应先发 gatewayId 内部指令, 存储在此，并放入 server.gpeers. 如果已存在，

	int HandleTargetMessage(uint32_t target, xx::Data_r& dr) {
        ResetTimeout(15s);  // 无脑续命一波
		if (target != 0xFFFFFFFF) return 1;	// passthrough
        std::string_view cmd;
        if (int r = dr.Read(cmd)) return -__LINE__; // 试读出 cmd。出错返回负数，掐线
        if (gatewayId != 0xFFFFFFFFu) {
            if (cmd == "accept"sv) {
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;
                std::string_view ip;
                if (int r = dr.Read(ip)) return -__LINE__;
                std::cout << "accept clientid = " << clientId << " ip = " << ip << std::endl;   // todo: check ip is valid?
                //(void)xx::Make<VPeer>(S, this, clientId, std::move(ip));
            }
            else if (cmd == "close"sv) {
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;
                //if (auto p = GetVPeerByClientId(clientId)) {
                //    p->Kick();
                //}
            }
            else if (cmd == "ping"sv) {
                Send(dr);   // echo back
            }
            else {
                std::cout << "unknown cmd = " << cmd << std::endl;
            }
        }
        else {
            if (cmd == "gatewayId"sv) {
                if (int r = dr.Read(gatewayId)) return -__LINE__;
                if (auto iter = server.gpeers.find(gatewayId); iter != server.gpeers.end()) return -__LINE__;   // 相同id已存在：掐线
                //S->gps[gatewayId] = xx::SharedFromThis(this);
                return;
            }
            else {
            }
        }
		return 0;
	}
	int ReceiveTargetRequest(uint32_t target, int32_t serial_, xx::ObjBase_s&& o_) {
		switch (o_.typeId()) {
		case xx::TypeId_v<Ping>: {
			auto&& o = o_.ReinterpretCast<Ping>();
			SendResponse<Pong>(0, serial_, o->ticks);
			break;
		}
		default:
			om.CoutN("ReceiveRequest unhandled package: ", o_);
		}
		om.KillRecursive(o_);
		return 0;
	}
};

int main() {
	Server server;
	server.Listen<GPeer>(54321);
	std::cout << "lobby + gateway + client -- lobby running... port = 54321" << std::endl;
	server.ioc.run();
	return 0;
}
