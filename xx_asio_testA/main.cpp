// lobby + gateway + client -- lobby
#include "xx_asio_codes.h"
#include "pkg.h"

struct GPeer;
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;
};

// 容器 创建，放入，移除 平时均由外部控制，避免造成 额外的引用 或者 不可控的生命周期
struct VPeer : xx::VPeerCode<VPeer, GPeer>, std::enable_shared_from_this<VPeer> {
    using VPC = xx::VPeerCode<VPeer, GPeer>;
    Server& server; // 指向总的上下文
    std::string clientIP; // 保存 gpeer 告知的 对端ip

    VPeer(Server& server_, GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : VPC(server_.ioc, server_.om, ownerPeer_, clientId_)
        , server(server_)
        , clientIP(clientIP_)
    {}
    void Start_() {
        ResetTimeout(15s);  // 设置初始的超时时长
        // todo: after accept logic here. 
    }

    // 收到 请求( 返回非 0 表示失败，会 Stop )
    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
        switch (o_.typeId()) {
        case xx::TypeId_v<Ping>: {
        	auto&& o = o_.ReinterpretCast<Ping>();
        	SendResponse<Pong>(serial, o->ticks);
        	break;
        }
        default:
        	om.CoutN("ReceiveRequest unhandled package: ", o_);
        }
        om.KillRecursive(o_);
        return 0;
    }

    // 收到 推送( 返回非 0 表示失败，会 Stop )
    int ReceivePush(xx::ObjBase_s&& o) {
        // todo: handle o
        om.KillRecursive(o);
        return 0;
    }

    // 通知网关延迟掐线 并 Stop. 调用前应先 Send 给客户端要收的东西
    void Kick(int64_t const& delayMS = 3000) {
        if (!Alive()) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));
        Stop();
    }

    // 从 owner 的 vpeers 中移除自己
    void RemoveFromOwner();

    // 切线
    void ResetOwner(GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_) {
        this->VPC::ResetOwner(ownerPeer_, clientId_);
        clientIP = clientIP_;
    }
};

// 来自网关的连接对端. 主要负责处理内部指令和 在 VPeer 之间 转发数据
struct GPeer : xx::PeerCode<GPeer>, xx::PeerTimeoutCode<GPeer>, xx::PeerHandleMessageCode<GPeer>, std::enable_shared_from_this<GPeer> {
	Server& server;
	GPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, server(server_)
	{
        ResetTimeout(15s);  // 设置初始的超时时长
    }
	uint32_t gatewayId = 0xFFFFFFFFu;	// gateway 连上来后应先发 gatewayId 内部指令, 存储在此，并放入 server.gpeers. 如果已存在，就掐线( 不顶下线 )
    std::unordered_map<uint32_t, std::shared_ptr<VPeer>> vpeers;

	// PeerHandleMessageCode 会在包切片后调用
	// 根据 target 找到 VPeer 转发 或执行 内部指令, 非 0xFFFFFFFFu 则为正常数据，走转发流，否则走内部指令
    int HandleData(xx::Data_r&& dr) {
        uint32_t target;
        if (int r = dr.ReadFixed(target)) return __LINE__;		// 试读取 target. 失败直接断开
        if (target != 0xFFFFFFFFu) {
            if (auto iter = vpeers.find(target); iter != vpeers.end() && iter->second->Alive()) {	//  试找到有效 vpeer 并转交数据
                iter->second->HandleData<true>(std::move(dr));  // 跳过 target 的处理
            }
        }
        else {	// 内部指令
            std::string_view cmd;
            if (int r = dr.Read(cmd)) return -__LINE__; // 试读出 cmd。出错返回负数，掐线
            if (cmd == "accept"sv) {
                uint32_t clientId;  // 试读出 clientId。出错返回负数，掐线
                if (int r = dr.Read(clientId)) return -__LINE__;
                std::string_view ip;    // 试读出 ip。出错返回负数，掐线
                if (int r = dr.Read(ip)) return -__LINE__;
                CreateVPeer(clientId, ip);
            }
            else if (cmd == "close"sv) {
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;
                if (auto iter = vpeers.find(clientId); iter != vpeers.end() && iter->second->Alive()) {
                    iter->second->Kick();
                }
            }
            else if (cmd == "ping"sv) {
                Send(dr);   // echo back
            }
            else if (cmd == "gatewayId"sv) {    // gateway 连上之后的首包, 注册自己
                if (int r = dr.Read(gatewayId)) return -__LINE__;
                if (auto iter = server.gpeers.find(gatewayId); iter != server.gpeers.end()) return -__LINE__;   // 相同id已存在：掐线
                server.gpeers[gatewayId] = shared_from_this();  // 放入容器备用
            }
            else {
                std::cout << "unknown cmd = " << cmd << std::endl;
            }
        }
        ResetTimeout(15s);  // 无脑续命
        return 0;
    }

    void Stop_() {
        for (auto& kv : vpeers) {
            kv.second->Stop();
        }
        vpeers.clear(); // 根据业务需求来。有可能 vpeer 在 stop 之后还会保持一段时间，甚至 重新激活
    }

    // 创建并插入 vpeers 一个 VPeer. 接着将执行其 Start_ 函数
    // 常用于服务之间 通知对方 通过 gatewayId + clientId 创建一个 vpeer 并继续后续流程. 比如 通知 游戏服务 某网关某玩家 要进入
    std::shared_ptr<VPeer> CreateVPeer(uint32_t const& clientId, std::string_view const& ip) {
        if (vpeers.find(clientId) != vpeers.end()) return {};
        auto& p = vpeers[clientId];
        p = std::make_shared<VPeer>(server, *this, clientId, ip);    // 放入容器备用
        p->Start();
        return p;
    }
};

void VPeer::RemoveFromOwner() {
    assert(ownerPeer);
    ownerPeer->vpeers.erase(clientId);
}

int main() {
	Server server;
	server.Listen<GPeer>(54321);
	std::cout << "lobby + gateway + client -- lobby running... port = 54321" << std::endl;
	server.ioc.run();
	return 0;
}
