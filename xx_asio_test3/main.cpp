// simple tcp test client for xx_asio_test2

#include <xx_asio_codes.h>
#include "ss.h"

// 适合以 Push 为主的，游戏客户端 按 帧 的方式访问. 收到的数据都会堆积在 peer 的 recvs
struct CPeer;
struct Client : xx::IOCCode<Client> {
	xx::ObjManager om;
	std::shared_ptr<CPeer> peer;																// 当前 peer
	operator bool() const;																		// peer && peer->Alive()

	std::string domain;																			// 域名/ip. 需要在 拨号 前填充
	uint16_t port = 0;																			// 端口. 需要在 拨号 前填充

	void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口
	void Update();																				// 每帧开始和逻辑结束时 call 一次
	awaitable<int> Dial();																		// 拨号. 成功连上返回 0
	void Reset();																				// 老 peer Stop + 新建 peer
	template<typename T = xx::ObjBase>
	xx::Shared<T> TryPopPackage();																// 尝试 move 出一条最前面的消息
	template<typename PKG = xx::ObjBase, typename ... Args>
	void Send(Args const& ... args);															// 试着用 peer 来 SendPush
};

struct CPeer : xx::PeerCode<CPeer>, xx::PeerTimeoutCode<CPeer>, xx::PeerRequestCode<CPeer>, std::enable_shared_from_this<CPeer> {
	Client& client;
	CPeer(Client& client_)
		: PeerCode(client_.ioc, asio::ip::tcp::socket(client_.ioc))
		, PeerTimeoutCode(client_.ioc)
		, PeerRequestCode(client_.om)
		, client(client_)
	{}
	std::deque<xx::ObjBase_s> recvs;															// 已收到的 push 数据包

	int ReceivePush(xx::ObjBase_s&& o_) {
		recvs.push_back(std::move(o_));															// 数据无脑往 recvs 放
		return 0;
	}
};

template<typename T>
inline xx::Shared<T> Client::TryPopPackage() {
	if (*this && !peer->recvs.empty()) {
		auto r = std::move(peer->recvs.front());
		peer->recvs.pop_front();
		if constexpr (std::is_same_v<T, xx::ObjBase>) return r;
		else if (r.typeId() == xx::TypeId_v<T>) return r.ReinterpretCast<T>();
	}
	return {};
}

inline void Client::SetDomainPort(std::string_view domain_, uint16_t port_) {
	domain = domain_;
	port = port_;
}

inline void Client::Update() {
	ioc.poll_one();
}

template<typename PKG, typename ... Args>
inline void Client::Send(Args const& ... args) {
	if (!peer || !peer->Alive()) return;
	peer->SendPush<PKG>(args...);																// 转发到 peer 的 SendPush 函数
}

inline Client::operator bool() const {
	return peer && peer->Alive();
}

inline void Client::Reset() {
	if (peer) {
		peer->Stop();
	}
	peer = std::make_shared<CPeer>(*this);														// 每次新建 容易控制 协程生命周期, 不容易出问题
}

inline awaitable<int> Client::Dial() {
	if (domain.empty()) co_return __LINE__;
	if (!port) co_return __LINE__;
	Reset();
	asio::ip::address addr;
	do {
		asio::ip::tcp::resolver resolver(ioc);													// 创建一个域名解析器
		auto rr = co_await(resolver.async_resolve(domain, ""sv, use_nothrow_awaitable) || xx::Timeout(5s));	// 开始解析, 得到一个 variant 包含两个协程返回值
		if (rr.index() == 1) co_return __LINE__;												// 如果 variant 存放的是第 2 个结果, 那就是 超时
		auto& [e, rs] = std::get<0>(rr);														// 展开第一个结果为 err + results
		if (e) co_return __LINE__;																// 出错: 解析失败
		auto iter = rs.cbegin();																// 拿到迭代器，指向首个地址
		if (auto idx = (int)((size_t)xx::NowEpochMilliseconds() % rs.size())) {					// 根据当前 ms 时间点 随机选一个下标
			std::advance(iter, idx);															// 快进迭代器
		}
		addr = iter->endpoint().address();														// 从迭代器获取地址
	} while (false);
	if (auto r = co_await peer->Connect(addr, port, 5s)) co_return __LINE__;					// 开始连接. 超时 5 秒
	co_return 0;
}

struct Logic {
    Client c;
	Logic() {
		co_spawn(c.ioc, [this]()->awaitable<void> {
			c.SetDomainPort("127.0.0.1", 12345);												// 填充域名和端口
		LabBegin:
			c.Reset();																			// 开始前先 reset + sleep 一把，避免各种协程未结束的问题
			co_await xx::Timeout(1s);
			if (auto r = co_await c.Dial()) {													// 域名解析并拨号. 失败就重来
				xx::CoutTN("Dial r = ", r);
				goto LabBegin;
			}
			c.Send<SS_C2S::Enter>();															// 发 enter 并等待收到 enter result 15 秒
			xx::Shared<SS_S2C::EnterResult> er;
			for (int i = 0; i < 150; ++i) {
				if (auto o = c.TryPopPackage<SS_S2C::EnterResult>()) {							// 强逻辑, 收到的包必须是指定类型，否则取出来是 empty
					er = std::move(o);															// 成功取出, 挪到 er 并退出等待
					break;
				}
				co_await xx::Timeout(100ms);													// 150 * 100ms = 15s
				if (!c) break;																	// 断线检测
			}
			if (!er) goto LabBegin;																// 超时: 重来

			c.om.CoutTN(er);																	// 等到了 enter result. 打印
			
			while(c) {																			// 等待 断开 并打印收到的东西
				if (auto o = c.TryPopPackage()) {
					c.om.CoutTN(o);
				}
				co_await xx::Timeout(100ms);
			};
			goto LabBegin;																		// 重来
		}, detached);
	}
};

int main() {
	Logic logic;
	do {
		logic.c.Update();																		// 每帧来一发
	    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));						// 模拟游戏循环延迟
	} while(true);
	xx::CoutN("end.");
	return 0;
}
