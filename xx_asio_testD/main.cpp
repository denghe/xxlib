// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// db
#include "xx_asio_codes.h"
#include "pkg.h"
#include "db.h"

struct Worker : xx::WorkerCode<Worker> {
	xx::ObjManager om;
	DB db;
};

struct LobbyPeer;
struct Game1Peer;
// ...
struct Server : xx::IOCCode<Server> {
	xx::ObjManager om;

	// workers
	uint32_t numWorkers = std::thread::hardware_concurrency() * 2;							// 获取 cpu 核心数, * 2 作为 db 工作线程数
	std::unique_ptr<Worker[]> workers = std::make_unique<Worker[]>(numWorkers);				// 无序操作专用. [workerCursor++] 循环访问
	uint32_t workerCursor = 0;																// worker 游标
	std::unique_ptr<Worker[]> orderedWorkers = std::make_unique<Worker[]>(numWorkers);		// 有序操作专用. 用相关 id % numWorkers 取模

	Worker& GetWork() {
		auto& w = workers[workerCursor++];
		if (workerCursor == numWorkers) {
			workerCursor = 0;
		}
		return w;
	}

	Worker& GetOrderedWork(int64_t const& id) {
		return orderedWorkers[id % numWorkers];
	}

	// server peers
	std::shared_ptr<LobbyPeer> lobbyPeer;
	std::unordered_set<std::shared_ptr<Game1Peer>> game1Peers;
	// ...
};

template<typename PeerDeriveType>
struct PeerBase : xx::PeerCode<PeerDeriveType>, xx::PeerRequestCode<PeerDeriveType>, std::enable_shared_from_this<PeerDeriveType> {
	using PC = xx::PeerCode<PeerDeriveType>;
	using PRC = xx::PeerRequestCode<PeerDeriveType>;
	Server& server;
	PeerBase(Server& server_, asio::ip::tcp::socket&& socket_)
		: PC(server_.ioc, std::move(socket_))
		, PRC(server_.om)
		, server(server_)
	{}

	// 将 f 放入 worker 执行，并于其执行完毕后 主线程 放行
	// F: void(Worker& w, xx::ObjBase_s& r)
	template<typename F>
	awaitable<void> HandleRequestCore(Worker& w, int32_t serial, F& f) {
		xx::Data d;																			// 待发数据容器
		asio::steady_timer t(PEERTHIS->ioc, std::chrono::steady_clock::time_point::max());	// 结果等待器
		w.ioc.post([&] {
			xx::ObjBase_s r;																// 结果容器
			f(w, r);																		// 用 worker thread 来执行 f
			if (r) {																		// 如果有数据要发
				d = xx::MakePackageData(w.om, serial, r);									// 将 serial, r 以 Response 格式 填入 d
				w.om.KillRecursive(r);														// 消除递归 便于析构
			}
			PEERTHIS->ioc.post([&] { t.cancel(); });										// main thread 结果等待器 放行
		});
		co_await t.async_wait(use_nothrow_awaitable);										// 等结果
		if (!d.len) co_return;																// 如果没有要发的东西, 直接退出
		PEERTHIS->Send(std::move(d));														// 回发处理结果
	}

	// 启动一个乱序工作协程处理请求。处理完毕后回发处理结果. 会加持 peer 智能指针, 复制序列号，移动 f 进协程。
	template<typename F>
	void HandleUnorderedRequest(int32_t serial, F&& f) {
		co_spawn(PEERTHIS->ioc, [this, self = PEERTHIS->shared_from_this(), serial, f = std::forward<F>(f)]()->awaitable<void> {
			co_await HandleRequestCore(server.GetWork(), serial, f);						// 参数为 乱序 worker.ioc
		}, detached);
	}

	// HandleRequest 的 顺序版。oid_ 为线程选择依据. 最好先获取到独立变量，再传递，避免 lambda move 影响 oid 所在容器( 靠传参顺序不稳定, gcc clang 表现出来不一致 )
	template<typename F>
	void HandleOrderedRequest(int32_t serial, uint64_t oid_, F&& f) {
		co_spawn(PEERTHIS->ioc, [this, self = PEERTHIS->shared_from_this(), oid_, serial, f = std::forward<F>(f)]()->awaitable<void> {
			co_await HandleRequestCore(server.GetOrderedWork(oid_), serial, f);				// 参数为 顺序 worker.ioc
		}, detached);
	}
};

struct LobbyPeer : PeerBase<LobbyPeer> {
	using PB = PeerBase<LobbyPeer>;
	using PB::PB;

	void Start_() {
		server.lobbyPeer = shared_from_this();
	}

	void Stop_() {
		server.lobbyPeer.reset();
	}

	// 收到 请求( 返回非 0 表示失败，会 Stop )
	int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
		server.om.CoutTN("LobbyPeer ReceiveRequest serial = ", serial, " o_ = ", o_);
		switch (o_.typeId()) {
		case xx::TypeId_v<All_Db::GetPlayerId>: {
			HandleRequest(serial, std::move(o_.ReinterpretCast<All_Db::GetPlayerId>()));
			break;
		}
		case xx::TypeId_v<All_Db::GetPlayerInfo>: {
			HandleRequest(serial, std::move(o_.ReinterpretCast<All_Db::GetPlayerInfo>()));
			break;
		}
		default:
			server.om.CoutN("LobbyPeer receive unhandled package: ", o_);
			om.KillRecursive(o_);
		}
		return 0;
	}

	void HandleRequest(int32_t serial, xx::Shared<All_Db::GetPlayerId>&& o) {
		HandleUnorderedRequest(serial, [o = std::move(o)](Worker& w, xx::ObjBase_s& r) {
			std::this_thread::sleep_for(500ms);											// 模拟 db 慢查询
			if (o->username == "a"sv && o->password == "1"sv) {
				auto v = xx::Make<Generic::Success>();
				v->value = 1;
				r = std::move(v);
			}
			else {
				auto v = xx::Make<Generic::Error>();
				v->number = 2;
				v->message = "bad username or password.";
				r = std::move(v);
			}
		});
	}

	void HandleRequest(int32_t serial, xx::Shared<All_Db::GetPlayerInfo>&& o) {
        auto oid = o->id;   // 独立写一行存下来 避免 o move 后取不到
		HandleOrderedRequest(serial, oid, [o = std::move(o)](Worker& w, xx::ObjBase_s& r) {
			std::this_thread::sleep_for(500ms);											// 模拟 db 慢查询
			if (o->id == 1) {
				auto v = xx::Make<Generic::PlayerInfo>();
				v->username = "a"sv;
				v->password = "1"sv;
				v->id = 1;
				v->nickname = "asdf";
				v->gold = 123;
				r = std::move(v);
			}
			else {
				auto v = xx::Make<Generic::Error>();
				v->number = 2;
				xx::Append(v->message, "can't find player id : ", o->id);
				r = std::move(v);
			}
		});
	}

};

struct Game1Peer : PeerBase<Game1Peer> {
	using PB = PeerBase<Game1Peer>;
	using PB::PB;

	void Start_() {
		server.game1Peers.insert(shared_from_this());
	}

	void Stop_() {
		server.game1Peers.erase(shared_from_this());
	}

	// 收到 请求( 返回非 0 表示失败，会 Stop )
	int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
		server.om.CoutTN("Game1Peer ReceiveRequest serial = ", serial, " o_ = ", o_);
		switch (o_.typeId()) {
		case xx::TypeId_v<All_Db::GetPlayerInfo>: {
			HandleRequest(serial, std::move(o_.ReinterpretCast<All_Db::GetPlayerInfo>()));
			break;
		}
		default:
			server.om.CoutN("Game1Peer receive unhandled package: ", o_);
			om.KillRecursive(o_);
		}
		return 0;
	}

	void HandleRequest(int32_t serial, xx::Shared<All_Db::GetPlayerInfo>&& o) {
        auto oid = o->id;
		HandleOrderedRequest(serial, oid, [o = std::move(o)](Worker& w, xx::ObjBase_s& r) {
			std::this_thread::sleep_for(500ms);											// 模拟 db 慢查询
			if (o->id == 1) {
				auto v = xx::Make<Generic::PlayerInfo>();
				v->username = "a"sv;
				v->password = "1"sv;
				v->id = 1;
				v->nickname = "asdf";
				v->gold = 123;
				r = std::move(v);
			}
			else {
				auto v = xx::Make<Generic::Error>();
				v->number = 2;
				xx::Append(v->message, "can't find player id : ", o->id);
				r = std::move(v);
			}
		});
	}
};

// ...

int main() {
	DB::MainInit();
	Server server;
	server.Listen<LobbyPeer>(55100);
	server.Listen<Game1Peer>(55101);
	// ...
	std::cout << "db running..." << std::endl;
	server.ioc.run();
	return 0;
}
