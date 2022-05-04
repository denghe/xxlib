// db       55100( for lobby )      55101( for game1 )      ...
// lobby    55000( for gateway )    55201( for game1 )      ...
// game1    55001( for gateway )
// gateway  54000( for client )

// db
#include "xx_asio_codes.h"
#include "pkg.h"

struct Worker : public asio::noncopyable {
	asio::io_context ioc;
	std::thread _thread;
	// more db ctx here
	Worker()
		: ioc(1)
		, _thread(std::thread([this]() { auto g = asio::make_work_guard(ioc); ioc.run(); }))
	{}
	~Worker() {
		ioc.stop();
		if (_thread.joinable()) {
			_thread.join();
		}
	}
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

	asio::io_context& GetWorkIOC() {
		auto& ioc = workers[workerCursor++].ioc;
		if (workerCursor == numWorkers) {
			workerCursor = 0;
		}
		return ioc;
	}

	asio::io_context& GetOrderedWorkIOC(int64_t const& id) {
		return orderedWorkers[id % numWorkers].ioc;
	}

	// server peers
	std::shared_ptr<LobbyPeer> lobbyPeer;
	std::shared_ptr<Game1Peer> game1Peer;
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

	// 等工作线程执行完毕后放行
	// F: void(xx::ObjBase_s& r)
	template<typename F>
	awaitable<void> HandleCore(asio::io_context& wioc, int32_t serial, F& f) {
		xx::ObjBase_s r;															// 结果容器
		asio::steady_timer t(PEERTHIS->ioc, std::chrono::steady_clock::time_point::max());	// 结果等待器
		wioc.post([&] {
			f(r);																	// 用 worker thread 来执行 f
			PEERTHIS->ioc.post([&] { t.cancel(); });								// main thread 结果等待器 放行
		});
		co_await t.async_wait(use_nothrow_awaitable);								// 等结果
		if (!r) co_return;															// 如果没有值 那就属于 ioc 直接 cancel, 直接退出
		if (!PEERTHIS->Alive()) co_return;											// 如果已断开，则无法发回( 逻辑有可能需要继续生效 )
		PEERTHIS->SendResponse(serial, r);											// 回发处理结果
	}

	// 启动一个乱序工作协程处理请求。处理完毕后回发处理结果. 会加持 peer 智能指针, 复制序列号，移动 f 进协程。
	template<typename F>
	void HandleRequest(int32_t serial, F&& f) {
		co_spawn(PEERTHIS->ioc, [this, self = PEERTHIS->shared_from_this(), serial, f = std::forward<F>(f)]()->awaitable<void> {
			co_await HandleCore(server.GetWorkIOC(), serial, f);
		}, detached);
	}

	// HandleRequest 的 顺序版。oid_ 为线程选择依据. 故意放最后以便优先从 要 move 到 f[] 的上下文获取值
	template<typename F>
	void HandleOrderedRequest(int32_t serial, F&& f, uint64_t oid_) {
		co_spawn(PEERTHIS->ioc, [this, self = PEERTHIS->shared_from_this(), oid_, serial, f = std::forward<F>(f)]()->awaitable<void> {
			co_await HandleCore(server.GetOrderedWorkIOC(oid_), serial, f);
		}, detached);
	}
};

struct LobbyPeer : PeerBase<LobbyPeer> {
	using PB = PeerBase<LobbyPeer>;
	using PB::PB;

	void Start_() {
		server.lobbyPeer = shared_from_this();
	}

	// 收到 请求( 返回非 0 表示失败，会 Stop )
	int ReceiveRequest(int32_t serial, xx::ObjBase_s&& o_) {
		switch (o_.typeId()) {
		case xx::TypeId_v<All_Db::GetPlayerId>: {
			auto&& o = o_.ReinterpretCast<All_Db::GetPlayerId>();
			HandleRequest(serial, [o = std::move(o)](xx::ObjBase_s& r) {
				std::this_thread::sleep_for(1s);										// 模拟 db 慢查询
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
			break;
		}
		case xx::TypeId_v<All_Db::GetPlayerInfo>: {
			auto&& o = o_.ReinterpretCast<All_Db::GetPlayerInfo>();
			HandleOrderedRequest(serial, [o = std::move(o)](xx::ObjBase_s& r) {
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
			}, o->id);
			break;
		}
		default:
			server.om.CoutN("receive unhandled package: ", o_);
			om.KillRecursive(o_);
		}
		return 0;
	}
};

struct Game1Peer : PeerBase<Game1Peer> {
	using PB = PeerBase<Game1Peer>;
	using PB::PB;

	void Start_() {
		server.game1Peer = shared_from_this();
	}
};

// ...

int main() {
	Server server;
	server.Listen<LobbyPeer>(55100);
	server.Listen<Game1Peer>(55101);
	// ...
	std::cout << "db running..." << std::endl;
	server.ioc.run();
	return 0;
}
