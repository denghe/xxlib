// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// game1
#include "xx_asio_codes.h"
#include "gpeer_code.h"
#include "pkg.h"

struct GPeer;                                                                       // G = Gateway
struct DPeer;                                                                       // D = Database
struct LPeer;                                                                       // L = Lobby
struct Server : xx::IOCCode<Server> {
    xx::ObjManager om;
    uint32_t serverId = 1;                                                          // 当前服务编号( 填充自 config )
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;                    // 网关 peer 集合. key 为配置的 网关编号
    std::shared_ptr<DPeer> dpeer = std::make_shared<DPeer>(*this);                  // 主动连接到 db server 的 peer
    std::shared_ptr<LPeer> lpeer = std::make_shared<LPeer>(*this);                  // 主动连接到 lobby server 的 peer
    void Run();
};

struct DPeer : xx::PeerCode<DPeer>, xx::PeerRequestCode<DPeer>, std::enable_shared_from_this<DPeer> {
    Server& server;
    DPeer(Server& server_)
        : PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
        , PeerRequestCode(server_.om)
        , server(server_)
    {}
};

struct LPeer : xx::PeerCode<LPeer>, xx::PeerRequestCode<LPeer>, std::enable_shared_from_this<LPeer> {
    Server& server;
    LPeer(Server& server_)
        : PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
        , PeerRequestCode(server_.om)
        , server(server_)
    {}

    // 连上后首包发 id 注册
    void Start_() {
        SendPush<Generic::Register>(server.serverId);
    }

    // 收到 目标的 推送( 返回非 0 表示失败，会 Stop )
    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_);
};

struct GPeer;
struct VPeer : xx::VPeerCode<VPeer, GPeer>, std::enable_shared_from_this<VPeer> {
    using VPC = xx::VPeerCode<VPeer, GPeer>;

    Server& server;
    VPeer(Server& server_, GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : VPC(server_.ioc, server_.om, ownerPeer_, clientId_, clientIP_)
        , server(server_) {}

    // todo: 双向绑定 上下文啥的?

    void Start_() {
        ResetTimeout(15s);                                                          // 设置初始的超时时长
    }

    // 通知网关延迟掐线 并 Stop. 调用前应先 Send 给客户端要收的东西
    void Kick(int64_t const& delayMS = 3000) {
        if (!Alive()) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));                     // 给 gateway 发 延迟掐线指令
        Stop();
    }

    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_);
};

struct GPeer : GPeerCode<GPeer, VPeer, Server>, std::enable_shared_from_this<GPeer> {
    using GPC = GPeerCode<GPeer, VPeer, Server>;
    using GPC::GPC;
};

int VPeer::ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
    //om.CoutTN("ReceiveRequest serial = ", serial, " o_ = ", o_);
    ResetTimeout(15s);                                                              // 无脑续命
    switch (o_.GetTypeId()) {
    case xx::TypeId_v<Ping>: {
        auto&& o = o_.ReinterpretCast<Ping>();                                      // 转为具体类型
        SendResponse<Pong>(serial, o->ticks);                                       // 回应结果
        break;
    }
                                                                                    // todo: enter ? leave? game cmd ?
    default:
        om.CoutTN("VPeer ReceiveRequest unhandled package: ", o_);
    }
    om.KillRecursive(o_);                                                           // 如果 数据包存在 循环引用, 则阻断. 避免 内存泄露
    return 0;
}


int LPeer::ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
    switch (o_.GetTypeId()) {
    case xx::TypeId_v<Lobby_Game1::PlayerEnter>: {
        auto&& o = o_.ReinterpretCast<Lobby_Game1::PlayerEnter>();
        auto giter = server.gpeers.find(o->gatewayId);
        if (giter == server.gpeers.end() || !giter->second->Alive()) {
            SendResponse<Generic::Error>(serial, __LINE__, "can't find gatewayId: "s + std::to_string(o->gatewayId));
            return 0;
        }

        //giter->second->CreateVPeer(o->clientId);
        // 
        //auto viter = giter->second->vpeers.find(o->clientId);
        //if (viter == giter->second->vpeers.end() || !viter->second->Alive()) {
        //    SendResponse<Generic::Error>(serial, __LINE__, "can't find gatewayId: "s + std::to_string(o->gatewayId) + " clientId: " + std::to_string(o->clientId));
        //    return 0;
        //}
        //o->gatewayId
        //o->clientId
        //o->playerId
        break;
    }
    }
    // todo: handle o_
    om.KillRecursive(o_);
    return 0;
}


void Server::Run() {
    xx::CoutTN("game1 running..."sv);
    co_spawn(ioc, [this]()->awaitable<void> {
        while (!ioc.stopped()) {                                                    // 自动连 db server
            if (dpeer->stoped) {												    // 如果没连上，就开始连. 
                if (int r = co_await dpeer->Connect(asio::ip::address::from_string("127.0.0.1"), 55101, 2s)) { // 开始连接. 超时 2 秒
                    xx::CoutTN("dpeer->Connect failed. r = ", r);
                }
            }
            co_await xx::Timeout(1000ms);									        // 避免无脑空转，省点 cpu
        }
        }, detached);
    co_spawn(ioc, [this]()->awaitable<void> {
        while (!ioc.stopped()) {                                                    // 自动连 lobby server
            if (lpeer->stoped) {												    // 如果没连上，就开始连. 
                if (int r = co_await lpeer->Connect(asio::ip::address::from_string("127.0.0.1"), 55201, 2s)) { // 开始连接. 超时 2 秒
                    xx::CoutTN("lpeer->Connect failed. r = ", r);
                }
            }
            co_await xx::Timeout(1000ms);									        // 避免无脑空转，省点 cpu
        }
        }, detached);
    ioc.run();
}

int main() {
    Server server;
    server.Listen<GPeer>(55000);
    server.Run();
    return 0;
}
