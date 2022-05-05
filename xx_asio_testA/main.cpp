﻿// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// lobby
#include "xx_asio_codes.h"
#include "pkg.h"

struct GPeer;                                                                       // G = Gateway
struct DPeer;                                                                       // D = Database
struct Game1Peer;
struct Server : xx::IOCCode<Server> {
    xx::ObjManager om;
    uint32_t serverId = 0;                                                          // 当前服务编号( 填充自 config )
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;                    // 网关 peer 集合. key 为配置的 网关编号
    std::shared_ptr<DPeer> dpeer = std::make_shared<DPeer>(*this);                  // 主动连接到 db server 的 peer
    std::unordered_map<uint32_t, std::shared_ptr<Game1Peer>> game1peers;            // 游戏服务 peer 集合. key 为配置的 服务编号
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

template<typename PeerDeriveType>
struct MPeerBase : xx::PeerCode<PeerDeriveType>, xx::PeerRequestCode<PeerDeriveType>, std::enable_shared_from_this<PeerDeriveType> {
    using PC = xx::PeerCode<PeerDeriveType>;
    using PRC = xx::PeerRequestCode<PeerDeriveType>;
    Server& server;
    uint32_t serverId = 0xFFFFFFFFu;	                                            // game server 连上来后应先发 serverId 内部指令, 存储在此，并放入 server.mpeers. 如果已存在，就掐线( 不顶下线 )
    MPeerBase(Server& server_, asio::ip::tcp::socket&& socket_)
        : PC(server_.ioc, std::move(socket_))
        , PRC(server_.om)
        , server(server_)
    {}
};

struct Game1Peer : MPeerBase<Game1Peer> {
    using MPeerBase::MPeerBase;
    int ReceivePush(xx::ObjBase_s&& o_) {
        if (o_.typeId() == xx::TypeId_v<Generic::Register>) {
            auto&& o = o_.ReinterpretCast<Generic::Register>();
            if (serverId != 0xFFFFFFFFu) {                                          // 如果不是初始值, 说明收到过了
                xx::CoutTN("MPeerBase ReceivePush dupelicate Generic::Register id = ", o->id);
                return -__LINE__;
            }
            xx::CoutTN("Game1Peer ReceivePush Generic::Register id = ", o->id);
            if (auto iter = server.game1peers.find(serverId); iter != server.game1peers.end()) return -__LINE__;   // 相同id已存在：掐线
            server.game1peers[serverId] = shared_from_this();                       // 放入容器备用
        }
        else {
            server.om.CoutTN("MPeerBase ReceivePush unhandled package : ", o_);
            om.KillRecursive(o_);
        }
        return 0;
    }

    void Stop_() {
        server.game1peers.erase(serverId);                                          // 从容器移除
    }
};

// 网关连接上来的 client 形成的 虚拟 peer. 令逻辑层感觉上是直连
struct VPeer : xx::VPeerCode<VPeer, GPeer>, std::enable_shared_from_this<VPeer> {
    using VPC = xx::VPeerCode<VPeer, GPeer>;
    Server& server;                                                                 // 指向总的上下文
    std::string clientIP;                                                           // 保存 gpeer 告知的 对端ip

    // 状态标记区. doing 表示正在做, 起到独占作用. done 表示已完成
    bool doingLogin = false;
    bool doneLogin = false;
    xx::Shared<Lobby_Client::Scene> scene;

    VPeer(Server& server_, GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : VPC(server_.ioc, server_.om, ownerPeer_, clientId_)
        , server(server_)
        , clientIP(clientIP_)
    {}
    void Start_() {
        ResetTimeout(15s);  // 设置初始的超时时长
        // todo: after accept logic here. 
        om.CoutTN("vpeer started... clientId = ", clientId, ", clientIP = ", clientIP);
    }

    // 收到 请求( 返回非 0 表示失败，会 Stop )
    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
        //om.CoutTN("ReceiveRequest serial = ", serial, " o_ = ", o_);
        ResetTimeout(15s);                                                          // 无脑续命
        switch (o_.typeId()) {
        case xx::TypeId_v<Ping>: {
            auto&& o = o_.ReinterpretCast<Ping>();                                  // 转为具体类型
            SendResponse<Pong>(serial, o->ticks);                                   // 回应结果
            break;
        }
        case xx::TypeId_v<Client_Lobby::Login>: {                                   // 注意: vpper Alive 和 dpeer Alive() 检查看情况，可以不做, 后果为 发不出去, SendRequest 立即返回空
            auto&& o = o_.ReinterpretCast<Client_Lobby::Login>();                   // 下面开始合法性检测
            if (doneLogin) {                                                        // 如果 Login 已完成，则本次指令认定为非法
                SendResponse<Generic::Error>(serial, __LINE__, "doneLogin == true"sv);
                break;
            }                                                                       // 如果 Login 已经正在发生，则本次指令认定为非法, 忽略或掐线都行
            if (doingLogin) {
                SendResponse<Generic::Error>(serial, __LINE__, "doingLogin == true"sv);
                break;
            }
            om.CoutTN("VPeer clientId = ", clientId, " ReceiveRequest o_ = ", o);
            doingLogin = true;                                                      // 启动协程之前先设置独占标记
            co_spawn(ioc, [this, self = shared_from_this(), serial, o = std::move(o)]()->awaitable<void> {
                auto sg = xx::MakeSimpleScopeGuard([&] { doingLogin = false; });    // 确保退出协程时清除独占标记
                if (auto r_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerId>(15s, o->username, o->password)) {
                    switch (r_.typeId()) {
                    case xx::TypeId_v<Generic::Error>: {                            // db 返回的错误信息 转发
                        auto&& r = r_.ReinterpretCast<Generic::Error>();
                        SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r->number) + ", message = " + r->message);
                        break;
                    }
                    case xx::TypeId_v<Generic::Success>: {                          // 成功. value 里面有 id
                        auto&& r = r_.ReinterpretCast<Generic::Success>();          // 下面继续获取玩家信息
                        if (auto r2_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerInfo>(15s, r->value)) {
                            switch (r2_.typeId()) {
                            case xx::TypeId_v<Generic::Error>: {                    // db 返回的错误信息 转发
                                auto&& r2 = r2_.ReinterpretCast<Generic::Error>();
                                SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r2->number) + ", message = " + r2->message);
                                break;
                            }
                            case xx::TypeId_v<Generic::PlayerInfo>: {               // db 返回 玩家信息: 更新 flag, 创建场景啥的 并返回 成功 + Push
                                auto&& r2 = r2_.ReinterpretCast<Generic::PlayerInfo>();
                                scene.Emplace();
                                scene->playerInfo = std::move(r2);
                                scene->gamesIntro = "hi " + scene->playerInfo->nickname + "! welcome to lobby!";
                                doneLogin = true;
                                SendResponse<Generic::Success>(serial, scene->playerInfo->id);  // 返回 成功 + id
                                SendPush(scene);                                    // 推送场景
                                break;
                            }
                            default:
                                SendResponse<Generic::Error>(serial, __LINE__, om.ToString("dpeer return unhandled package: ", r2_));
                            }
                        }
                        else {
                            SendResponse<Generic::Error>(serial, __LINE__, "dpeer SendRequest All_Db::GetPlayerInfo timeout"sv);
                        }
                        break;
                    }
                    default:
                        SendResponse<Generic::Error>(serial, __LINE__, om.ToString("dpeer return unhandled package: ", r_));
                    }
                }
                else {
                    SendResponse<Generic::Error>(serial, __LINE__, "dpeer SendRequest All_Db::GetPlayerId timeout"sv);
                }
                // -- co_spawn
            }, detached);
            break;
        }
        default:
            om.CoutTN("VPeer ReceiveRequest unhandled package: ", o_);
        }
        om.KillRecursive(o_);                                                       // 如果 数据包存在 循环引用, 则阻断. 避免 内存泄露
        return 0;
    }

    // 通知网关延迟掐线 并 Stop. 调用前应先 Send 给客户端要收的东西
    void Kick(int64_t const& delayMS = 3000) {
        if (!Alive()) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));                     // 给 gateway 发 延迟掐线指令
        Stop();
    }

    // 从 owner 的 vpeers 中移除自己
    void RemoveFromOwner();

    // 切线: 重新设置 ownerPeer, clientId, clientIP 啥的, 复用 VPeer 上下文, 应对 顶下线 等需求
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
        if (int r = dr.ReadFixed(target)) return __LINE__;		                    // 试读取 target. 失败直接断开
        if (target != 0xFFFFFFFFu) {
            //xx::CoutTN("HandleData target = ", target, " dr = ", dr);
            if (auto iter = vpeers.find(target); iter != vpeers.end() && iter->second->Alive()) {	//  试找到有效 vpeer 并转交数据
                iter->second->HandleData(std::move(dr));                            // 跳过 target 的处理
            }
        }
        else {	// 内部指令
            std::string_view cmd;
            if (int r = dr.Read(cmd)) return -__LINE__;                             // 试读出 cmd。出错返回负数，掐线
            if (cmd == "accept"sv) {                                                // gateway accept client socket
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;                    // 试读出 clientId。出错返回负数，掐线
                std::string_view ip;
                if (int r = dr.Read(ip)) return -__LINE__;                          // 试读出 ip。出错返回负数，掐线
                xx::CoutTN("GPeer HandleData cmd = ", cmd, ". clientId = ", clientId, " ip = ", ip);
                CreateVPeer(clientId, ip);                                          // 创建相应的 VPeer
                Send(xx::MakeCommandData("open"sv, clientId));                      // 下发 open 指令
            }
            else if (cmd == "close"sv) {                                            // gateway client socket 已断开
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;
                xx::CoutTN("GPeer HandleData cmd = ", cmd, ", clientId = ", clientId);
                if (auto iter = vpeers.find(clientId); iter != vpeers.end()) {      // 找到就 Stop 并移除
                    iter->second->Stop();
                    vpeers.erase(iter);
                }
            }
            else if (cmd == "ping"sv) {                                             // 来自 网关 的 ping. 直接 echo 回去
                //xx::CoutTN("HandleData cmd = ", cmd);
                Send(dr);
            }
            else if (cmd == "gatewayId"sv) {                                        // gateway 连上之后的首包, 注册自己
                if (gatewayId != 0xFFFFFFFFu) {                                     // 如果不是初始值, 说明收到过了
                    xx::CoutTN("GPeer HandleData dupelicate cmd = ", cmd);
                    return -__LINE__;
                }
                if (int r = dr.Read(gatewayId)) return -__LINE__;
                xx::CoutTN("GPeer HandleData cmd = ", cmd, ", gatewayId = ", gatewayId);
                if (auto iter = server.gpeers.find(gatewayId); iter != server.gpeers.end()) return -__LINE__;   // 相同id已存在：掐线
                server.gpeers[gatewayId] = shared_from_this();                      // 放入容器备用
            }
            else {
                std::cout << "GPeer HandleData unknown cmd = " << cmd << std::endl;
            }
        }
        ResetTimeout(15s);                                                          // 无脑续命
        return 0;
    }

    void Stop_() {
        for (auto& kv : vpeers) {
            kv.second->Stop();                                                      // 停止所有虚拟peer
        }
        vpeers.clear(); // 根据业务需求来。有可能 vpeer 在 stop 之后还会保持一段时间，甚至 重新激活
        server.gpeers.erase(gatewayId);                                             // 从容器移除
    }

    // 创建并插入 vpeers 一个 VPeer. 接着将执行其 Start_ 函数
    // 常用于服务之间 通知对方 通过 gatewayId + clientId 创建一个 vpeer 并继续后续流程. 比如 通知 游戏服务 某网关某玩家 要进入
    std::shared_ptr<VPeer> CreateVPeer(uint32_t const& clientId, std::string_view const& ip) {
        if (vpeers.find(clientId) != vpeers.end()) return {};
        auto& p = vpeers[clientId];
        p = std::make_shared<VPeer>(server, *this, clientId, ip);                   // 放入容器备用
        p->Start();
        return p;
    }
};

void VPeer::RemoveFromOwner() {
    assert(ownerPeer);
    ownerPeer->vpeers.erase(clientId);
}

void Server::Run() {
    xx::CoutTN("lobby running..."sv);
    co_spawn(ioc, [this]()->awaitable<void> {
        while (!ioc.stopped()) {                                                    // 自动连 db server
            if (dpeer->stoped) {												    // 如果没连上，就开始连. 
                if (int r = co_await dpeer->Connect(asio::ip::address::from_string("127.0.0.1"), 55100, 2s)) { // 开始连接. 超时 2 秒
                    xx::CoutTN("dpeer->Connect r = ", r);
                }
                else {
                    xx::CoutTN("dpeer connected.");
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
    server.Listen<Game1Peer>(55201);
    server.Run();
    return 0;
}
