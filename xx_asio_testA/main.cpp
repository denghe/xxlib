// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// lobby

#include "xx_asio_codes.h"
#include "gpeer_code.h"
#include "pkg.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
//#include <boost/shared_ptr.hpp>
using boost::multi_index_container;
using namespace boost::multi_index;

namespace tags {
    struct id {};
    struct username {};
    struct lastActiveTime {};
}
typedef multi_index_container<xx::Shared<Generic::PlayerInfo>, 
    indexed_by<
    hashed_unique<tag<tags::id>, BOOST_MULTI_INDEX_MEMBER(Generic::PlayerInfo, int64_t, id)>,
    hashed_unique<tag<tags::username>, BOOST_MULTI_INDEX_MEMBER(Generic::PlayerInfo, std::string, username)>,
    ordered_non_unique<tag<tags::lastActiveTime>, BOOST_MULTI_INDEX_MEMBER(Generic::PlayerInfo, int64_t, lastActiveTime)>
>> OnlinePlayers;

struct GPeer;                                                                           // G = Gateway
struct DPeer;                                                                           // D = Database
struct Game1Peer;
struct VPeer;                                                                           // V = Virtual 代表 玩家连接 的虚拟 peer

struct Server : xx::IOCCode<Server> {
    xx::ObjManager om;
    uint32_t serverId = 0;                                                              // 当前服务编号( 填充自 config )
    bool running = true;                                                                // 运行标记, 用于通知各种退出
    std::unordered_map<uint32_t, std::shared_ptr<GPeer>> gpeers;                        // 网关 peer 集合. key 为配置的 网关编号
    std::shared_ptr<DPeer> dpeer = std::make_shared<DPeer>(*this);                      // 主动连接到 db server 的 peer
    std::unordered_map<uint32_t, std::shared_ptr<Game1Peer>> game1peers;                // 游戏服务 peer 集合. key 为配置的 服务编号

    /*
玩家状态管理

玩家数据位于 数据库 和 内存

玩家状态有以下几种：( 这些状态之间互斥 )
    加载: 从db读入玩家信息. 之后将转为 在线 状态
    在线: 玩家信息存放于内存. 如果一段时间内玩家没有 活动, 将转为 卸载 状态
    卸载: 玩家信息从内存写入db并清理内存. 之后如果有玩家 登录，将转为 加载 状态

玩家的 登录 行为，可能触发 加载（如果不 在线 ），也可能无法立即回应（正在 加载 或 卸载）。需要等这些步骤完成才能继续
*/

    std::unordered_set<std::string> loadingPlayers;                                     // 加载 中的 玩家 用户名 集合( 单项 生命周期 由协程 管理 )
    OnlinePlayers players;                                                              // 在线 玩家集合
    std::unordered_set<std::string> unloadingPlayers;                                   // 卸载 中的 玩家 用户名 集合. 挨个通知 db 保存，直到成功，最后 清理( 单项 生命周期 由协程 管理 )

    // 通过 username & password 拿玩家 id
    // 先去 players 找 username。如果有，直接对比 password, 正确则返回 id
    // 如果 未找到，继续找 unloadingPlayers. 如果 在里面，就等到 它不见为止
    // 继续找 loadingPlayers, 如果 在里面，就等到 它不见为止
    // 最后 去 players ( 同第一步 )
    // 用户名 & 密码对比 成功, 返回 智能指针. 密码对比失败 或 异常退出，返回 null
    awaitable<xx::Shared<Generic::PlayerInfo>> GetPlayer(std::string const& username, std::string const& password);

    void Run();                                                                         // 启几个定时器，实现 自动连 db, 以及 清理 超时 player，最后持续执行服务
};

struct DPeer : xx::PeerCode<DPeer>, xx::PeerRequestCode<DPeer>, std::enable_shared_from_this<DPeer> {
    Server& server;
    DPeer(Server& server_)
        : PeerCode(server_.ioc, asio::ip::tcp::socket(server_.ioc))
        , PeerRequestCode(server_.om)
        , server(server_)
    {}
};

struct Game1Peer : xx::PeerCode<Game1Peer>, xx::PeerRequestCode<Game1Peer>, std::enable_shared_from_this<Game1Peer> {
    Server& server;
    uint32_t serverId = 0xFFFFFFFFu;
    Game1Peer(Server& server_, asio::ip::tcp::socket&& socket_)
        : PeerCode(server_.ioc, std::move(socket_))
        , PeerRequestCode(server_.om)
        , server(server_) {}

    int ReceivePush(xx::ObjBase_s&& o_) {
        if (serverId == 0xFFFFFFFFu) {                                                  // 当前为未注册状态, 只期待 Generic::Register 数据包. 
            if (o_.typeId() == xx::TypeId_v<Generic::Register>) {                       // game server 应先发首包 Generic::Register, 自报家门. id 并放入 server.mpeers. 如果已存在，就掐线( 不顶下线 )
                auto&& o = o_.ReinterpretCast<Generic::Register>();
                if (o->id == 0xFFFFFFFFu) {                                             // 简单检查 id 值域, 如果为表达 未填写的特殊值, 报错退出
                    xx::CoutTN("Game1Peer ReceivePush Generic::Register error. bad id range: id = ", o->id);
                    return -__LINE__;
                }
                serverId = o->id;                                                       // 存 id
                if (auto iter = server.game1peers.find(serverId); iter != server.game1peers.end()) {    // id 已存在：当前连接 掐线( 不把旧连接顶下线 )
                    xx::CoutTN("Game1Peer ReceivePush Generic::Register error. id already exists: id = ", o->id);
                    return -__LINE__;   
                }
                server.game1peers[serverId] = shared_from_this();                       // 放入容器备用
                xx::CoutTN("Game1Peer ReceivePush Generic::Register success. id = ", o->id);
            }
        }
        else {
            server.om.CoutTN("GamePeerBase ReceivePush unhandled package : ", o_);
            om.KillRecursive(o_);
        }
        return 0;
    }

    void Stop_() {
        server.game1peers.erase(serverId);                                              // 从容器移除
    }
};

struct VPeer : xx::VPeerCode<VPeer, GPeer>, std::enable_shared_from_this<VPeer> {
    Server& server;
    VPeer(Server& server_, GPeer& ownerPeer_, uint32_t const& clientId_, std::string_view const& clientIP_)
        : VPeerCode(server_.ioc, server_.om, ownerPeer_, clientId_, clientIP_)
        , server(server_)
    {}

    bool doingLogin = false;                                                            // 是否正在登录

    xx::Shared<Generic::PlayerInfo> playerInfo;                                         // 和 玩家信息 双向 bind

    void Start_() {
        ResetTimeout(15s);                                                              // 设置初始的超时时长
    }
    void Stop_() {
        if (playerInfo) {
            playerInfo->vpeer.reset();                                                  // 断线时解绑
        }
    }

    void Kick(int64_t const& delayMS = 3000) {                                          // 通知网关延迟掐线 并 Stop. 调用前应先 Send 给客户端要收的东西
        if (!Alive()) return;
        Send(xx::MakeCommandData("kick"sv, clientId, delayMS));                         // 给 gateway 发 延迟掐线指令
        Stop();
    }

    int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
        //om.CoutTN("ReceiveRequest serial = ", serial, " o_ = ", o_);
        ResetTimeout(15s);                                                              // 给连接续命
        if (playerInfo) {
            playerInfo->lastActiveTime = xx::NowSteadyEpochSeconds();                   // 更新最后活动时间点
        }
        if (o_.typeId() == xx::TypeId_v<Ping>) {
            SendResponse<Pong>(serial, o_.ReinterpretCast<Ping>()->ticks);              // 回应结果
            return 0;
        }
        if (playerInfo) {                                                               // 已登录状态
            switch (o_.typeId()) {
            case xx::TypeId_v<Client_Lobby::EnterGame>: {
                auto&& o = o_.ReinterpretCast<Client_Lobby::EnterGame>();               // 收到 进入游戏 的请求

                if (o->gamePath.empty()) {                                              // 开始参数 check
                    //xx::CoutTN("VPeer ReceiveRequest Client_Lobby::EnterGame error. gamePath is empty. player id = ", );
                    return -__LINE__;
                }
                // todo: 定位一台 game server 并通知对方 玩家要进入, 等收到结果后 回复 玩家 game server id

                break;
            }
            default: {
                om.CoutTN("VPeer ReceiveRequest unhandled package: ", o_);
            }
            }
        }
        else {                                                                          // 未登录状态
            switch (o_.typeId()) {
            case xx::TypeId_v<Client_Lobby::Login>: {                                   // 注意: vpper Alive 和 dpeer Alive() 检查看情况，可以不做, 后果为 发不出去, SendRequest 立即返回空
                auto&& o = o_.ReinterpretCast<Client_Lobby::Login>();                   // 下面开始合法性检测
                if (playerInfo) {                                                       // 如果 Login 已完成，则本次指令认定为非法
                    SendResponse<Generic::Error>(serial, __LINE__, "playerInfo not null"sv);
                    break;
                }                                                                       // 如果 Login 已经正在发生，则本次指令认定为非法, 忽略或掐线都行
                if (doingLogin) {
                    SendResponse<Generic::Error>(serial, __LINE__, "doingLogin == true"sv);
                    break;
                }
                om.CoutTN("VPeer clientId = ", clientId, " ReceiveRequest o_ = ", o);

                doingLogin = true;                                                      // 启动协程之前先设置独占标记
                co_spawn(ioc, [this, self = shared_from_this(), serial, o = std::move(o)]()->awaitable<void> {
                    auto sg1 = xx::MakeSimpleScopeGuard([&] {                           // 确保退出协程时清除独占标记
                        doingLogin = false;
                        });

                LabBegin:
                    {
                        auto pi = co_await server.GetPlayer(o->username, o->password);  // 根据 un pw 定位用户( 如果正在 加载 卸载, 会一直等 )
                        if (!Alive() || !server.running) co_return;                     // 如果已经 掉线 或服务 停止 就直接退出
                        if (pi) {
                            if (auto oldvpeer = playerInfo->vpeer.lock()) {             // 开始顶下线逻辑。如果存在老连接
                                // todo: 这里可以 send 被顶下线的信息通知
                                oldvpeer->Kick();                                       // 踢掉
                            }
                            playerInfo = std::move(pi);                                 // 开始双向绑定
                            playerInfo->vpeer = shared_from_this();
                            playerInfo->lastActiveTime = xx::NowSteadyEpochSeconds();
                            co_return;
                        }
                    }

                    // 经过上面的步骤，已确认内存里没有。先向 loadingPlayers 压入 用户名
                    auto result = server.loadingPlayers.emplace(o->username);
                    if (!result.second) goto LabBegin;                                  // 压入失败，已存在。跳到上一个步骤去等结果

                    auto sg2 = xx::MakeSimpleScopeGuard([&] {                           // 确保退出协程时清除 loading 状态
                        server.loadingPlayers.erase(o->username);
                        });

                    // 去 db 获取 id
                    if (auto r_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerId>(15s, o->username, o->password)) {
                        switch (r_.typeId()) {
                        case xx::TypeId_v<Generic::Error>:                              // db 返回的错误信息 转发
                        {
                            auto&& r = r_.ReinterpretCast<Generic::Error>();
                            SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r->number) + ", message = " + r->message);
                            break;
                        }
                        case xx::TypeId_v<Generic::Success>:                            // 成功. value 里面有 id
                        {
                            auto&& r = r_.ReinterpretCast<Generic::Success>();

                            // 去 db 获取 玩家信息
                            if (auto r2_ = co_await server.dpeer->SendRequest<All_Db::GetPlayerInfo>(15s, r->value)) {
                                switch (r2_.typeId()) {
                                case xx::TypeId_v<Generic::Error>:                      // db 返回的错误信息 转发
                                {
                                    auto&& r2 = r2_.ReinterpretCast<Generic::Error>();
                                    SendResponse<Generic::Error>(serial, __LINE__, "dpeer return error: number = "s + std::to_string(r2->number) + ", message = " + r2->message);
                                    break;
                                }
                                case xx::TypeId_v<Generic::PlayerInfo>:                 // db 返回 玩家信息: 创建各种上下文 并返回 成功 + Push
                                {
                                    auto&& r2 = r2_.ReinterpretCast<Generic::PlayerInfo>();

                                    auto rtv = server.players.insert(r2);               // 向 players 插入
                                    if (!rtv.second) {                                  // 插入失败( 理论上讲不可能 )
                                        SendResponse<Generic::Error>(serial, __LINE__, om.ToString("server.players.insert(r2) failed. r2 = ", r2));
                                        break;
                                    }
                                    playerInfo = r2;                                    // 插入成功。开始双向 bind
                                    playerInfo->vpeer = shared_from_this();
                                    playerInfo->lastActiveTime = xx::NowSteadyEpochSeconds();

                                    auto& s = playerInfo->scene.Emplace();              // 创建个人场景
                                    s->gamesIntro = "hi " + playerInfo->nickname + "! welcome to lobby! avaliable game id list = { 1, 2 }"; // 继续填充场景内容
                                    server.om.CloneTo(playerInfo, s->playerInfo);       // 以 clone 的方式 填充 info 防止递归引用. clone 并不涉及 生成物 之外的 .inc 附加部分

                                    SendResponse<Generic::Success>(serial, playerInfo->id);  // 返回 成功 + id
                                    SendPush(s);                                        // 推送场景
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
            default:
            {
                om.CoutTN("VPeer ReceiveRequest unhandled package: ", o_);
            }
            }
        }
        om.KillRecursive(o_);                                                       // 如果 数据包存在 循环引用, 则阻断. 避免 内存泄露
        return 0;
    }
};

struct GPeer : GPeerCode<GPeer, VPeer, Server>, std::enable_shared_from_this<GPeer> {
    using GPeerCode::GPeerCode;
};

inline awaitable<xx::Shared<Generic::PlayerInfo>> Server::GetPlayer(std::string const& username, std::string const& password) {
LabBegin:
    auto&& c = players.get<tags::username>();
    if (auto r = c.find(username); r != c.end()) {
        co_return (*r)->password == password ? (*r) : xx::Shared<Generic::PlayerInfo>{};
    }
    while (true) {
        if (!running) co_return xx::Shared<Generic::PlayerInfo>{};
        if (!unloadingPlayers.contains(username)) break;
        co_await xx::Timeout(100ms);
    }
    while (true) {
        if (!running) co_return xx::Shared<Generic::PlayerInfo>{};
        if (!loadingPlayers.contains(username)) break;
        co_await xx::Timeout(100ms);
    }
    goto LabBegin;
}

inline void Server::Run() {
    xx::CoutTN("lobby running..."sv);

    // 自动连 db server
    co_spawn(ioc, [this]()->awaitable<void> {
        while (running) {
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

    // 定时扫所有 player 最后行为时间点. 如果 小于 （当前时间点 - 超时时长) 那就该 做 下线 处理
    co_spawn(ioc, [this]()->awaitable<void> {
        auto&& c = players.get<tags::lastActiveTime>();
        while (running) {
            co_await xx::Timeout(50ms);									            // 避免无脑空转，省点 cpu. 但也不能间隔太久，这样会导致 db 瞬间压力大
            auto tp = (int64_t)xx::NowEpochSeconds() - 60;                          // 算出超时时间点( 60 可来自配置 )

            for (auto it = c.cbegin(); it != c.cend();) {
                auto& p = (*it);
                if (tp > p->lastActiveTime) {
                    auto r = unloadingPlayers.emplace(p->username);                 // 在 unloadingPlayers 中注册 username
                    assert(r.second);
                    
                    co_spawn(ioc, [this, p = p]()->awaitable<void> {                // 创建一个协程 向 db 提交数据
                        auto sg = xx::MakeSimpleScopeGuard([&] {                    // 退出时自动从 队列 移除
                            unloadingPlayers.erase(p->username);
                        });
                        while (true) {
                            if (!running) {
                                om.CoutTN("player info store to db failed: ", p);
                            }
                            // todo
                            co_return;  // 先模拟存盘成功
                            //if (auto r2_ = co_await dpeer->SendRequest<All_Db::SavePlayerData>(15s, p)) {
                            //    switch (r2_.typeId()) {
                            //    case xx::TypeId_v<Generic::Success>: co_return;
                            //    default:
                            //        om.CoutTN("player info store to db failed: ", r2_);
                            //    }
                            //}
                            co_await xx::Timeout(100ms);
                        }
                        }, detached);

                    it = c.erase(it);                                               // 从在线列表移除, it 定位到下一个对象
                } else {
                    break;  // ++it;                                                // 因为是 有序队列，这里不再需要继续往下扫, 后面的时间都没有超
                }
            }
        }
        // todo: 退出时的 check 逻辑, 补存盘逻辑 啥的 ??
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
