// simple tcp test client for xx_asio_test2

#include <xx_asio_codes.h>
#include "ss.h"

// 适合以 Push 为主的，游戏客户端 按 帧 的方式访问. 收到的数据都会堆积在 peer 的 recvs
struct CPeer;
struct Client : xx::IOCCode<Client> {
	xx::ObjManager om;
	std::shared_ptr<CPeer> peer;																// 当前 peer
	bool busy = false;																			// 繁忙标志位
	operator bool() const;																		// peer && peer->Alive()

	std::string domain;																			// 域名/ip. 需要在 拨号 前填充
	uint16_t port = 0;																			// 端口. 需要在 拨号 前填充

	void SetDomainPort(std::string_view domain_, uint16_t port_);								// 填充 域名/ip, 端口
	void Update();																				// 每帧开始和逻辑结束时 call 一次
	void Dial();																				// 开始拨号
	void Reset();																				// 老 peer Stop + 新建 peer
	xx::ObjBase_s TryGetPackage();																// 尝试 move 出一条最前面的消息
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

inline xx::ObjBase_s Client::TryGetPackage() {
	if (*this && !peer->recvs.empty()) {
		auto r = std::move(peer->recvs.front());
		peer->recvs.pop_front();
		return r;
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

inline void Client::Dial() {
	if (busy) return;
	if (!domain.empty()) return;
	if (!port) return;
	busy = true;
	Reset();
	co_spawn(ioc, [this]()->awaitable<void> {
		auto sg = xx::MakeSimpleScopeGuard([&] { busy = false; });								// 自动释放 busy flag
		asio::ip::address addr;
		do {
			asio::ip::tcp::resolver resolver(ioc);												// 创建一个域名解析器
			auto rr = co_await(resolver.async_resolve(domain, use_nothrow_awaitable) || xx::Timeout(5s));	// 开始解析, 得到一个 variant 包含两个协程返回值
			if (rr.index() == 1) co_return;														// 如果 variant 存放的是第 2 个结果, 那就是 超时
			auto& [e, rs] = std::get<0>(rr);													// 展开第一个结果为 err + results
			if (e) co_return;																	// 出错: 解析失败
			auto iter = rs.cbegin();															// 拿到迭代器，指向首个地址
			if (auto idx = (int)((size_t)xx::NowEpochMilliseconds() % rs.size())) {				// 根据当前 ms 时间点 随机选一个下标
				std::advance(iter, idx);														// 快进迭代器
			}
			addr = iter->endpoint().address();													// 从迭代器获取地址
		} while (false);
		if (auto r = co_await peer->Connect(addr, port, 5s)) co_return;							// 开始连接. 超时 5 秒
	}, detached);
}


struct Logic {
    Client c;
    double secs = 0;
    int lineNumber = 0;
    int Update() {
        c.Update();

        COR_BEGIN
        // 初始化拨号地址
        c.SetDomainPort("192.168.1.95", 12345);

    LabBegin:
        xx::CoutTN("LabBegin");
        // 无脑重置一发
        c.Reset();

        // 睡一秒
        secs = xx::NowEpochSeconds() + 1;
        do {
            COR_YIELD;
        } while (secs > xx::NowEpochSeconds());

        xx::CoutTN("Dial");
        // 开始拨号
		c.Dial();

        // 等 busy
        do {
            COR_YIELD;
        } while (c.busy);

        // 没连上: 重来
        if (!c) goto LabBegin;

        xx::CoutTN("Send");
        // 发 进入 请求
        {
            auto o = xx::Make<SS_C2S::Enter>();
            c.Send(o);
        }

        xx::CoutTN("keep alive");
        // 如果断线就重连
        do {
            COR_YIELD;
			if (auto o = c.TryGetPackage()) {
                c.om.CoutN(o);
            }
        } while (c);
        goto LabBegin;
        COR_END
    }
};

int main() {
	Logic c;
	do {
	    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
	} while((c.lineNumber = c.Update()));
	xx::CoutN("end.");
	return 0;
}
