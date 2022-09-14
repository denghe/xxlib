// simple tcp server for shooter game. cocos client src path = shoot_game_bak

#include "xx_asio_codes.h"
#include "ss.h"

struct CPeer;                                                                                   // C = Client
struct Server : xx::IOCCode<Server> {
    Server();

    xx::ObjManager om;
	uint32_t pid = 0;																            // 客户端连接id 自增量, 产生 peer 时++填充
	std::unordered_map<uint32_t, std::shared_ptr<CPeer>> ps;						            // 客户端连接 集合

	xx::Shared<SS::Scene> scene;                                                                // 游戏场景上下文
	xx::Shared<SS_S2C::Sync> sync;                                                              // 用于下发完整场景
	xx::Shared<SS_S2C::Event> event;                                                            // 用于下发事件
	std::unordered_set<uint32_t> newEnters;                                                     // 记录本帧即将需要处理的新进入玩家的 peer id

	int64_t nowMS = xx::NowSteadyEpochMilliseconds();                                           // 当前毫秒
	int64_t lastMS = 0;                                                                         // 上次执行 FrameUpdate 的毫秒
	double totalDelta = 0;                                                                      // 已经历的时间池( 稳帧算法 )
	int64_t keepAliveMS = 0;                                                                    // 确保定时下发 event( 即便没有也下发, 辅助客户端同步 )
    int FrameUpdate(int64_t nowMS);                                                             // 帧逻辑
    void Kick(uint32_t clientId);                                                               // 从 peer 和场景 移除玩家, 并生成事件
    void SendEvent();                                                                           // 给所有人下发事件
	int Run();                                                                                  // 开始阻塞执行
};

struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer>, std::enable_shared_from_this<CPeer> {
	Server& server;
	CPeer(Server& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, PeerTimeoutCode(server_.ioc)
		, PeerRequestCode(server_.om)
		, server(server_)
        , clientId(++server_.pid)
	{}

    uint32_t clientId;                                                                          // key
    std::queue<xx::ObjBase_s> recvs;                                                            // 收到的所有包

    int ReceivePush(xx::ObjBase_s&& o_) {                                                       // 收包处理
        switch (o_.GetTypeId()) {
        case xx::TypeId_v<SS_C2S::Enter>: {                                                     // 响应 进入游戏 请求
            om.CoutTN("clientId = ", clientId, " recv package: ", o_);
            if (server.ps.contains(clientId)) return __LINE__;                                  // 重复发起 Enter ? 掐线
            server.ps.insert({ clientId, shared_from_this() });                                 // 将 peer 插入 server.ps
            server.newEnters.insert(clientId);                                                  // 放入 newEnters 以便在 FrameUpdate 中创建逻辑对象
            SendPush<SS_S2C::EnterResult>(clientId);                                            // 回发处理结果
            ResetTimeout(15s);                                                                  // 重置超时时长
            break;
        }
        case xx::TypeId_v<SS_C2S::Cmd>: {                                                       // 响应 操作指令
            om.CoutTN("clientId = ", clientId, " recv package: ", o_);
            recvs.push(std::move(o_));                                                          // 压入队列
            if (recvs.size() > 30) return __LINE__;                                             // 如果短时间内收到很多，导致累积到一定量，掐线
            ResetTimeout(15s);                                                                  // 续命
            break;
        }
        default:                                                                                // 收到 意料之外的 包
            om.CoutTN("clientId = ", clientId, " recv unhandled package: ", o_);
            om.KillRecursive(o_);                                                               // 确保递归部分内存回收( 如果有的话 )
            return __LINE__;
        }
        return 0;
    }

    void Stop_() {                                                                              // 断线时调用 kick 逻辑
        server.Kick(clientId);
        clientId = 0;
    }
};

Server::Server() {                                                                              // 各种初始化
    scene.Emplace();
    sync.Emplace();
    sync->scene = scene;
    event.Emplace();

    totalDelta = 0;
    lastMS = xx::NowSteadyEpochMilliseconds();
    keepAliveMS = lastMS + 1000;
}

int Server::Run() {
	Listen<CPeer>(12345);                                                                       // 开始监听 tcp socket 连入
	co_spawn(ioc, [this]()->awaitable<void> {                                                   // 启动一个游戏循环协程
		while (!ioc.stopped()) {                                                                // todo: 退出条件
            auto nowMS = xx::NowSteadyEpochMilliseconds();
            auto d = (double)(nowMS - lastMS) / 1000.;                                          // 计算出时间差( 秒 )
            totalDelta += d;                                                                    // 累积时间池
            if (int r = FrameUpdate(nowMS)) co_return;                                          // 调用帧逻辑
            if (d < (1. / 60.)) {                                                               // 如果执行时间很短，就安排休眠
                co_await xx::Timeout(1ms);                                                      // 协程短暂休眠 省点 cpu
            }
            lastMS = nowMS;                                                                     // 更新时间上下文
		}
	}, detached);
    ioc.run();
	return 0;
}

int Server::FrameUpdate(int64_t nowMS) {
    if (totalDelta > (1.f / 60.f)) {                                                            // 如果满足一帧的运行条件( 已经历的时长 >= 1 帧间隔 )
        if (!newEnters.empty()) {                                                               // 如果本次有刚进入的玩家
            auto d = xx::MakePackageData(om, 0, sync);                                          // 提前准备 完整同步 的下发数据
            for (auto& cid : newEnters) {
                if (auto iter = ps.find(cid); iter != ps.end()) {                               // 定位到 peer
                    if (auto& p = iter->second; p->Alive()) {                                   // 判断是否已断开( 理论上讲 一旦断开就不应该在 ps 里了 )
                        p->Send(xx::Data(d));                                                   // 下发 完整同步
                        auto&& shooter = scene->shooters[cid].Emplace();                        // 创建逻辑对象并进一步初始化
                        shooter->clientId = cid;                                                // todo: 进一步初始化 random pos & angle
                        event->enters.push_back(om.Clone(shooter));                             // 将数据 clone 到事件( 这步属于偷懒, 正常情况应该是下发初始化参数 )
                        shooter->scene = scene;                                                 // 最后引用 scene. 避免 clone 的时候牵连到 scene
                    }
                }
            }
            newEnters.clear();
        }
    }
    while (totalDelta > (1.f / 60.f)) {                                                         // 循环消耗时间池的时长
        totalDelta -= (1.f / 60.f);
        for (auto& kv : ps) {                                                                   // 遍历所有连接
            auto& recvs = kv.second->recvs;                                                     // 定位到 recvs
            if (!recvs.empty()) {                                                               // 如果队列不空
                auto& o = recvs.front().ReinterpretCast<SS_C2S::Cmd>();                         // 定位到头部
                assert(scene->shooters.contains(kv.first));                                     // todo: 数据校验
                auto& shooter = scene->shooters[kv.first];                                      // 定位到 shooter
                if (shooter->cs != o->cs) {                                                     // 如果 cmd 的内容 和 shooter 身上的 不一致：
                    shooter->cs = o->cs;                                                        // 应用差异( 模拟玩家输入 )
                    event->css.emplace_back(kv.first, o->cs);                                   // 产生事件
                }
                recvs.pop();                                                                    // 弹出头部。不再循环, 每帧处理一个 cmd
            }
        }
        if (!event->enters.empty() || !event->css.empty() || !event->quits.empty()) {           // 只要存在任何一种事件通知，就下发 并 清空
            SendEvent();
            event->quits.clear();
            event->enters.clear();
            event->css.clear();
            keepAliveMS = nowMS + 1000;                                                         // 重置 keep alive 超时 ms
        }
        if (int r = scene->Update()) return r;                                                  // 游戏主逻辑更新
    }
    if (nowMS > keepAliveMS && !ps.empty()) {                                                   // 如果 keep alive 超时，不管有没有 event 都下发
        SendEvent();
        keepAliveMS = nowMS + 1000;                                                             // 重置 keep alive 超时 ms
    }
    return 0;
}

void Server::SendEvent() {
    event->frameNumber = scene->frameNumber;
    auto d = xx::MakePackageData(om, 0, event);
    for (auto& kv : ps) {
        kv.second->Send(xx::Data(d));   // copy
    }
}

void Server::Kick(uint32_t clientId) {
    ps.erase(clientId);                                                                         // 从 peer 容器移除
    scene->shooters.erase(clientId);                                                            // 从场景逻辑移除
    event->quits.push_back(clientId);                                                           // 产生事件
}

int main() {
	Server s;
	return s.Run();
}
