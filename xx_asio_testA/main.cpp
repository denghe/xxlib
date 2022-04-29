// lobby + gateway + client -- lobby
#include "xx_asio_codes.h"
#include "pkg.h"

struct GPeer;
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;
};

struct VPeer : xx::PeerTimeoutCode<GPeer>, xx::PeerRequestCode<GPeer, true>, std::enable_shared_from_this<GPeer> {
    Server& server; // 指向总的上下文
    std::weak_ptr<GPeer> gpeer; // 指向父容器
    uint32_t clientId;  // 保存 gpeer 告知的 对端id，通信时作为 target 填充
    std::string clientIP; // 保存 gpeer 告知的 对端ip
    bool stoped = false;
    VPeer(Server& server_, std::shared_ptr<GPeer>& gpeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : PeerTimeoutCode(server_.ioc)
        , PeerRequestCode(server_.om)
        , server(server_)
        , gpeer(gpeer_)
        , clientId(clientId_)
        , clientIP(clientIP_)
    {
        ResetTimeout(15s);  // 设置初始的超时时长
    }
    void Start() {
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

    void Stop() {
        if (stoped) return;
        stoped = true;
        timeouter.cancel();
        for (auto& kv : reqs) {
            kv.second.first.cancel();
        }
    }

    bool Alive() const {
        return !stoped && !stoping;
    }

    // 通知 gateway 延迟掐线，顺便告知原因
    void Kick(int64_t const& delayMS = 3000) {
        if (stoped) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));
        Stop();
    }

    // 下列函数 包了一层，均通过 gpeer 路由发出数据
    void Send(xx::Data&& msg);

    template<typename PKG = xx::ObjBase, typename ... Args>
    awaitable<xx::ObjBase_s> SendRequest(std::chrono::steady_clock::duration d, Args const& ... args);

    template<typename PKG = xx::ObjBase, typename ... Args>
    void SendResponse(int32_t const& serial, Args const &... args);

    template<typename PKG = xx::ObjBase, typename ... Args>
    void SendPush(Args const& ... args);
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
                server.gpeers[gatewayId] = shared_from_this();
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
    }

    // 常用于服务之间 通知对方 通过 gatewayId + clientId 创建一个 vpeer 并继续后续流程. 比如 通知 游戏服务 某网关某玩家 要进入
    std::shared_ptr<VPeer> CreateVPeer(uint32_t const& clientId, std::string_view const& ip) {
        if (vpeers.find(clientId) != vpeers.end()) return {};
        auto& p = vpeers[clientId];
        //p = std::make_shared<VPeer>(server, shared_from_this(), clientId, ip);    // 放入容器备用
        p->Start(); // 模拟 Accept 事件调用
        return p;
    }
};

void VPeer::Send(xx::Data&& msg) {
    if (stoped) return;
    auto g = gpeer.lock();
    assert(g);
    assert(g->Alive());
    g->Send(std::move(msg));
}

template<typename PKG, typename ... Args>
awaitable<xx::ObjBase_s> VPeer::SendRequest(std::chrono::steady_clock::duration d, Args const& ... args) {
    if (stoped) return;
    auto g = gpeer.lock();
    assert(g);
    assert(g->Alive());
    co_return this->PeerRequestCode::SendRequest<PKG>(clientId, d, args...);
}

template<typename PKG, typename ... Args>
void VPeer::SendResponse(int32_t const& serial, Args const &... args) {
    if (stoped) return;
    auto g = gpeer.lock();
    assert(g);
    assert(g->Alive());
    this->PeerRequestCode::SendResponse(clientId, serial, args...);
}

template<typename PKG, typename ... Args>
void VPeer::SendPush(Args const& ... args) {
    if (stoped) return;
    auto g = gpeer.lock();
    assert(g);
    assert(g->Alive());
    this->PeerRequestCode::SendPush(clientId, args...);
}

int main() {
	Server server;
	server.Listen<GPeer>(54321);
	std::cout << "lobby + gateway + client -- lobby running... port = 54321" << std::endl;
	server.ioc.run();
	return 0;
}
