// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// lobby
#include "xx_asio_codes.h"
#include "gpeer_code.h"
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
struct GamePeerBase : xx::PeerCode<PeerDeriveType>, xx::PeerRequestCode<PeerDeriveType>, std::enable_shared_from_this<PeerDeriveType> {
    using PC = xx::PeerCode<PeerDeriveType>;
    using PRC = xx::PeerRequestCode<PeerDeriveType>;
    Server& server;
    uint32_t serverId = 0xFFFFFFFFu;
    GamePeerBase(Server& server_, asio::ip::tcp::socket&& socket_)
        : PC(server_.ioc, std::move(socket_))
        , PRC(server_.om)
        , server(server_)
    {}
};

struct Game1Peer : GamePeerBase<Game1Peer> {
    using GamePeerBase::GamePeerBase;
    int ReceivePush(xx::ObjBase_s&& o_) {
        if (serverId == 0xFFFFFFFFu) {                                              // 当前为未注册状态, 只期待 Generic::Register 数据包. 
            if (o_.typeId() == xx::TypeId_v<Generic::Register>) {                   // game server 应先发首包 Generic::Register, 自报家门. id 并放入 server.mpeers. 如果已存在，就掐线( 不顶下线 )
                auto&& o = o_.ReinterpretCast<Generic::Register>();
                if (o->id == 0xFFFFFFFFu) {                                         // 简单检查 id 值域, 如果为表达 未填写的特殊值, 报错退出
                    xx::CoutTN("Game1Peer ReceivePush Generic::Register error. bad id range: id = ", o->id);
                    return -__LINE__;
                }
                serverId = o->id;                                                   // 存 id
                if (auto iter = server.game1peers.find(serverId); iter != server.game1peers.end()) {    // id 已存在：当前连接 掐线( 不把旧连接顶下线 )
                    xx::CoutTN("Game1Peer ReceivePush Generic::Register error. id already exists: id = ", o->id);
                    return -__LINE__;   
                }
                server.game1peers[serverId] = shared_from_this();                   // 放入容器备用
                xx::CoutTN("Game1Peer ReceivePush Generic::Register success. id = ", o->id);
            }
        }
        else {
            // todo: switch case
            server.om.CoutTN("GamePeerBase ReceivePush unhandled package : ", o_);
            om.KillRecursive(o_);
        }
        return 0;
    }

    void Stop_() {
        server.game1peers.erase(serverId);                                          // 从容器移除
    }
};

struct GPeer;
struct VPeer : xx::VPeerCode<VPeer, GPeer>, std::enable_shared_from_this<VPeer> {
    using VPC = xx::VPeerCode<VPeer, GPeer>;
    using VPC::VPC;
    Server& server;
    VPeer(Server& server_, GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : VPC(server_.ioc, server_.om, ownerPeer_, clientId_, clientIP_)
        , server(server_)
    {}

    // todo: 连接上下文。多步骤操作上下文存在 peer 还是 player 上需要斟酌
    // 
    // 状态标记区. doing 表示正在做, 起到独占作用. done 表示已完成
    bool doingLogin = false;
    bool doneLogin = false;

    xx::Shared<Lobby_Client::Scene> scene;

    void Start_() {
        ResetTimeout(15s);                                                          // 设置初始的超时时长
    }

    void Kick(int64_t const& delayMS = 3000) {                                      // 通知网关延迟掐线 并 Stop. 调用前应先 Send 给客户端要收的东西
        if (!Alive()) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));                     // 给 gateway 发 延迟掐线指令
        Stop();
    }

    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_);                         // 处理 client 的 request
};

struct GPeer : GPeerCode<GPeer, VPeer, Server>, std::enable_shared_from_this<GPeer> {
    using GPC = GPeerCode<GPeer, VPeer, Server>;
    using GPC::GPC;
};

inline int VPeer::ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
    //om.CoutTN("ReceiveRequest serial = ", serial, " o_ = ", o_);
    ResetTimeout(15s);                                                              // 无脑续命
    switch (o_.typeId()) {
    case xx::TypeId_v<Ping>: {
        auto&& o = o_.ReinterpretCast<Ping>();                                      // 转为具体类型
        SendResponse<Pong>(serial, o->ticks);                                       // 回应结果
        break;
    }
    case xx::TypeId_v<Client_Lobby::Login>: {                                       // 注意: vpper Alive 和 dpeer Alive() 检查看情况，可以不做, 后果为 发不出去, SendRequest 立即返回空
        auto&& o = o_.ReinterpretCast<Client_Lobby::Login>();                       // 下面开始合法性检测
        if (doneLogin) {                                                            // 如果 Login 已完成，则本次指令认定为非法
            SendResponse<Generic::Error>(serial, __LINE__, "doneLogin == true"sv);
            break;
        }                                                                           // 如果 Login 已经正在发生，则本次指令认定为非法, 忽略或掐线都行
        if (doingLogin) {
            SendResponse<Generic::Error>(serial, __LINE__, "doingLogin == true"sv);
            break;
        }
        om.CoutTN("VPeer clientId = ", clientId, " ReceiveRequest o_ = ", o);
        doingLogin = true;                                                          // 启动协程之前先设置独占标记
        co_spawn(ioc, [this, self = shared_from_this(), serial, o = std::move(o)]()->awaitable<void> {
            auto sg = xx::MakeSimpleScopeGuard([&] { doingLogin = false; });    // 确保退出协程时清除独占标记
            if (auto r_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerId>(15s, o->username, o->password)) {
                switch (r_.typeId()) {
                case xx::TypeId_v<Generic::Error>: {                                // db 返回的错误信息 转发
                    auto&& r = r_.ReinterpretCast<Generic::Error>();
                    SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r->number) + ", message = " + r->message);
                    break;
                }
                case xx::TypeId_v<Generic::Success>: {                              // 成功. value 里面有 id
                    auto&& r = r_.ReinterpretCast<Generic::Success>();              // 下面继续获取玩家信息
                    if (auto r2_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerInfo>(15s, r->value)) {
                        switch (r2_.typeId()) {
                        case xx::TypeId_v<Generic::Error>: {                        // db 返回的错误信息 转发
                            auto&& r2 = r2_.ReinterpretCast<Generic::Error>();
                            SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r2->number) + ", message = " + r2->message);
                            break;
                        }
                        case xx::TypeId_v<Generic::PlayerInfo>: {                   // db 返回 玩家信息: 更新 flag, 创建场景啥的 并返回 成功 + Push
                            auto&& r2 = r2_.ReinterpretCast<Generic::PlayerInfo>();
                            scene.Emplace();
                            scene->playerInfo = std::move(r2);
                            scene->gamesIntro = "hi " + scene->playerInfo->nickname + "! welcome to lobby! avaliable game id list = { 1, 2 }";
                            doneLogin = true;
                            SendResponse<Generic::Success>(serial, scene->playerInfo->id);  // 返回 成功 + id
                            SendPush(scene);                                        // 推送场景
                            break;
                        }
                        default:
                            SendResponse<Generic::Error>(serial, __LINE__, om.ToString("dpeer return unhandled package: ", r2_));
                        }
                    } else {
                        SendResponse<Generic::Error>(serial, __LINE__, "dpeer SendRequest All_Db::GetPlayerInfo timeout"sv);
                    }
                    break;
                }
                default:
                    SendResponse<Generic::Error>(serial, __LINE__, om.ToString("dpeer return unhandled package: ", r_));
                }
            } else {
                SendResponse<Generic::Error>(serial, __LINE__, "dpeer SendRequest All_Db::GetPlayerId timeout"sv);
            }
            // -- co_spawn
        }, detached);
        break;
    }
    case xx::TypeId_v<Client_Lobby::EnterGame>: {
        auto&& o = o_.ReinterpretCast<Client_Lobby::EnterGame>();                   // 转为具体类型
        if (o->gamePath.empty()) {
            //xx::CoutTN("VPeer ReceiveRequest Client_Lobby::EnterGame error. gamePath is empty. player id = ", );
            return -__LINE__;
        }
        //SendResponse<Pong>(serial, o->ticks);                                       // 回应结果
        break;
    }
    default:
        om.CoutTN("VPeer ReceiveRequest unhandled package: ", o_);
    }
    om.KillRecursive(o_);                                                           // 如果 数据包存在 循环引用, 则阻断. 避免 内存泄露
    return 0;
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
