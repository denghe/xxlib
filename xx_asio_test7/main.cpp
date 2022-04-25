// asio simple telnet server for test features

#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <vector>
#include <iostream>
#include <string>
#include <string_view>
#include <charconv>
#include <utility>
#include <chrono>
#include <asio.hpp>
using namespace std::literals;
using namespace std::literals::chrono_literals;
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

namespace xx {

	asio::awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
		asio::steady_timer t(co_await asio::this_coro::executor);
		t.expires_after(d);
		co_await t.async_wait(use_nothrow_awaitable);
	}

	template<typename T>
	concept PeerDeriveType = requires(T t) {
		t.shared_from_this();	// struct PeerDeriveType : std::enable_shared_from_this< PeerDeriveType >
		t.StartAfter();			// 便于在 Start 尾部追加别的处理代码
		t.StopAfter();			// 便于在 Stop 尾部追加别的处理代码
		t.HandleMessage();		// 核心消息处理函数
	};

	template<typename PeerDeriveType>
	struct PeerCode : asio::noncopyable {
		asio::io_context& ioc;
		asio::ip::tcp::socket socket;
		asio::steady_timer writeBlocker;
		std::deque<std::string> writeQueue;
		bool stoped = true;

		PeerCode(asio::io_context& ioc_, asio::ip::tcp::socket&& socket_)
			: ioc(ioc_)
			, socket(std::move(socket_))
			, writeBlocker(ioc_)
		{
		}

		void Start() {
			if (!stoped) return;
			stoped = false;
			writeBlocker.expires_at(std::chrono::steady_clock::time_point::max());
			writeQueue.clear();
			((PeerDeriveType*)(this))->StartAfter();
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Read(); }, asio::detached);
			asio::co_spawn(ioc, [self = ((PeerDeriveType*)(this))->shared_from_this()]{ return self->Write(); }, asio::detached);
		}

		void Stop() {
			if (stoped) return;
			stoped = true;
			socket.close();
			writeBlocker.cancel();
			((PeerDeriveType*)(this))->StopAfter();
		}

		// for client dial connect to server only
		asio::awaitable<int> Connect(asio::ip::address const& ip, uint16_t port, std::chrono::steady_clock::duration d = 5s) {
			if (!stoped) co_return 1;
			auto r = co_await(socket.async_connect({ ip, port }, use_nothrow_awaitable) || Timeout(d));
			if (r.index()) co_return 2;
			if (auto& [e] = std::get<0>(r); e) co_return 3;
			Start();
			co_return 0;
		}

		void Send(std::string&& msg) {
			if (stoped) return;
			writeQueue.emplace_back(std::move(msg));
			writeBlocker.cancel_one();
		}

	protected:
		asio::awaitable<void> Read() {
			for (std::string buf;;) {
				auto [ec, n] = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf, 1024), "\n", use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (n > 1) {
					if (auto siz = n - (buf[n - 2] == '\r' ? 2 : 1)) {
						((PeerDeriveType*)(this))->HandleMessage({ buf.data(), siz });
						if (stoped) co_return;
					}
				}
				buf.erase(0, n);
			}
			Stop();
		}

		asio::awaitable<void> Write() {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					co_await writeBlocker.async_wait(use_nothrow_awaitable);
					if (stoped) co_return;
				}
				auto& msg = writeQueue.front();
				auto buf = msg.data();
				auto len = msg.size();
			LabBegin:
				auto [ec, n] = co_await asio::async_write(socket, asio::buffer(buf, len), use_nothrow_awaitable);
				if (ec) break;
				if (stoped) co_return;
				if (n < len) {
					len -= n;
					buf += n;
					goto LabBegin;
				}
				writeQueue.pop_front();
			}
			Stop();
		}
	};

	template<typename ServerDeriveType>
	struct ServerCode : asio::noncopyable {
		asio::io_context ioc;				// .run() execute
		asio::signal_set signals;
		ServerCode() : ioc(1), signals(ioc, SIGINT, SIGTERM) { }

		// 需要目标 Peer 实现( Server&, &&socket ) 构造函数
		template<typename Peer>
		void Listen(uint16_t port) {
			asio::co_spawn(ioc, [this, port]()->asio::awaitable<void> {
				asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
				for (;;) {
					asio::ip::tcp::socket socket(ioc);
					if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
						std::make_shared<Peer>(*(ServerDeriveType*)this, std::move(socket))->Start();
					}
				}
				}, asio::detached);
		}
	};

	// todo: more xxxxCode here
}





/***********************************************************************************************************/
/* logic
* 简单模拟一个 mud 服务器. 连上后，如果 ?? 秒内不发送 有效数据，就 踢掉
* 回车确认发送信息。发送错误将看到提示
* 有效数据有 2 两种：
* 	1. 指令: 单词 + 空格 + 参数...
* 	2. 回应: 数字 + 空格 + 回应内容...
*/


struct MyPeer;
struct MyServer : xx::ServerCode<MyServer> {

	// 指令处理函数集合
	std::unordered_map<std::string, std::function<int(MyPeer& p, std::string_view const& args)>> msgHandlers;

	// 玩家集合
	std::unordered_set<std::shared_ptr<MyPeer>> players;

	MyServer();
};

struct MyPeer : xx::PeerCode<MyPeer>, std::enable_shared_from_this<MyPeer> {
	// 引用到宿主 & ioc for easy use
	MyServer& server;
	asio::io_context& ioc;

	// 业务自增序号
	int32_t autoSerial = 0;

	// 客户端的回应 临时容器. 发起请求时先插入 自增序号, 阻塞器到此. 收到回应时, 写入 string 并令 timer 解除阻塞
	std::unordered_map<int, std::pair<asio::steady_timer, std::string>> reqs;

	// 逻辑行为超时检测用 timer
	asio::steady_timer timeouter;

	// 防重入
	bool lockLogin = false;
	bool playing = false;

	// for DelayStop
	bool stoping = false;

	// 配合 Listen 的构造需求
	MyPeer(MyServer& server_, asio::ip::tcp::socket&& socket_)
		: PeerCode(server_.ioc, std::move(socket_))
		, server(server_)
		, ioc(server_.ioc)
		, timeouter(ioc)
	{
		ResetTimeout(20s);
	}

	void StartAfter() {}

	// 创建一个 req 上下文并返回 iter. 顺便重置一下超时
	auto CreateReq() {
		return reqs.emplace(++autoSerial, std::make_pair(asio::steady_timer(ioc, std::chrono::steady_clock::time_point::max()), "")).first;
	}

	// Stop 的同时取消所有 timers
	void StopAfter() {
		timeouter.cancel();
		for (auto& kv : reqs) {
			kv.second.first.cancel();
		}
	}

	// 延迟 Stop
	void DelayStop(std::chrono::steady_clock::duration const& d) {
		if (stoping || stoped) return;
		stoping = true;
		ResetTimeout(d);
	}

	// 重置超时时长
	void ResetTimeout(std::chrono::steady_clock::duration const& d) {
		timeouter.expires_after(d);
		timeouter.async_wait([this](auto&& ec) {
			if (ec) return;
			Stop();
			});
	}


	// 消息路由( 投递进来的不会是 0 长度 )
	void HandleMessage(std::string_view const& msg) {
		// delay close 状态不处理任何读到的东西
		if (stoping) return;

		// 找一下空格，方便切割出首个 word / number
		auto n = msg.find_first_of(' ');

		// 根据第一个字母判断性质
		if (msg[0] >= '0' && msg[0] <= '9') {

			// 如果数字，就走序列号流程
			if (n == std::string_view::npos || n + 1 == msg.size()) {
				Send(std::string("invalid message: only number? ") + std::string(msg) + "\r\n");
				return;
			}

			// 数字转为 序号
			int serial;
			auto r = std::from_chars(msg.data(), msg.data() + n, serial);
			if (r.ec == std::errc::invalid_argument || r.ec == std::errc::result_out_of_range) {
				Send(std::string("invalid message: serial( string to int ) failed. ") + std::string(msg) + "\r\n");
				return;
			}

			// 判断字典里是否能找到
			auto iter = reqs.find(serial);
			if (iter == reqs.end()) {
				Send(std::string("invalid message: serial = ") + std::to_string(serial) + " can't find. " + std::string(msg) + "\r\n");
				return;
			}

			// 存储 内容 并放行协程
			iter->second.second = msg.substr(n + 1);
			iter->second.first.cancel();
		}
		else {
			// 单词, 走指令流程. 找到处理函数就 call
			auto word = std::string(msg.substr(0, n));
			if (auto iter = server.msgHandlers.find(word); iter != server.msgHandlers.end()) {
				iter->second(*this, msg.substr(n + 1));
			}
			else {
				Send("avaliable commands: login, stop ( stop play ), quit ( delaystop 3s ), exit ( direct stop ).\r\n");
			}
		}
	}
};


template<class F>
auto MakeSimpleScopeGuard(F&& f) noexcept {
	struct SG { F f; SG(F&& f) noexcept : f(std::move(f)) {} ~SG() { f(); } };
	return SG(std::forward<F>(f));
}


struct Client : xx::PeerCode<Client>, std::enable_shared_from_this<Client> {
	asio::io_context& ioc;
	asio::steady_timer recvBlocker;
	std::vector<std::string> recvs;

	Client(asio::io_context& ioc_)
		: PeerCode(ioc_, asio::ip::tcp::socket(ioc_))
		, ioc(ioc_)
		, recvBlocker(ioc_, std::chrono::steady_clock::time_point::max())
	{
	}

	void StartAfter() {}

	void StopAfter() {
		recvBlocker.cancel();
	}

	void HandleMessage(std::string_view const& msg) {
		recvs.emplace_back(std::string(msg));
		recvBlocker.cancel_one();
	}
};


MyServer::MyServer() {

	msgHandlers.insert({ "login", [this](MyPeer& p, std::string_view const& args) {
		// 模拟一个和玩家的交互流程

		// 逻辑锁检查
		if (p.lockLogin) {
			p.Send("invalid message: login( locked ).\r\n");
			return 0;
		}
		if (p.playing) {
			p.Send("invalid message: login( playing ).\r\n");
			return 0;
		}

		// 进入协程前加锁, 避免时间差边界问题
		p.lockLogin = true;

		// 重置超时时长
		p.ResetTimeout(15s);

		// 起个协程, 开始问答逻辑
		asio::co_spawn(ioc,[this, p = p.shared_from_this()]()->asio::awaitable<void> {

			// 退出函数自动解锁
			auto sgLock = MakeSimpleScopeGuard([&] { p->lockLogin = false; });

			// 对客户端发起请求
			auto iter = p->CreateReq();

			// 下发请求文字( 前置业务号 )
			p->Send(std::to_string(iter->first) + " | what's your name?\r\n");

			// 开始等待
			co_await iter->second.first.async_wait(use_nothrow_awaitable);

			// 这里不关心 timer 退出原因, 直接检查 string 内容. 如果空的那就是超时
			auto& s = iter->second.second;
			if (s.empty()) co_return;

			// todo: 模拟和 player 上下文的 bind

			// 续命
			p->ResetTimeout(20s);

			// 打招呼
			p->Send(std::string("hi ") + s + " !\r\n");

			// 花几秒发送几个倒数
			asio::steady_timer delay(ioc);
			for (int i = 5; i >= 0; --i) {
				p->Send(std::to_string(i) + "\r\n");
				delay.expires_after(1s); co_await delay.async_wait(use_nothrow_awaitable);
			}

			// 模拟进入了游戏场景，开始收到帧推送
			players.insert(p);

			// 标记正在玩的状态
			p->playing = true;

		}, asio::detached);
		return 0;
	} });

	msgHandlers.insert({ "stop", [this](MyPeer& p, std::string_view const& args) {
		if (!p.playing) {
			p.Send("invalid message: stop( !playing ).\r\n");
		}
		else {
			p.ResetTimeout(20s);
			p.playing = false;
			players.erase(std::static_pointer_cast<MyPeer>(p.shared_from_this()));
		}
		return 0;
	} });

	msgHandlers.insert({ "quit", [this](MyPeer& p, std::string_view const& args) {
		p.Send("bye!\r\n");
		if (p.playing) {
			p.playing = false;
			players.erase(std::static_pointer_cast<MyPeer>(p.shared_from_this()));
		}
		p.DelayStop(3s);
		return 0;
	} });

	msgHandlers.insert({ "exit", [this](MyPeer& p, std::string_view const& args) {
		p.Stop();
		return 1;
	} });

	// 模拟一个固定帧率的游戏循环。玩家 peer 如果纳入 players 就能收到 "广播". 如果玩家还有别的上下文信息，可以和 peer 双向关联
	asio::co_spawn(ioc, [this]()->asio::awaitable<void> {
		int frameNumber = 0;
		asio::steady_timer delay(ioc);
		while (true) {
			++frameNumber;
			auto iter = players.begin();
			while (iter != players.end()) {
				if (!(*iter)->stoped) {
					(*iter)->Send(std::to_string(frameNumber) + "\r");
					++iter;
				}
				else {
					iter = players.erase(iter);
				}
			}
			delay.expires_after(10ms); co_await delay.async_wait(use_nothrow_awaitable);	// yield 10ms
		}
		}, asio::detached);

	// 模拟一个客户端连上来
	asio::co_spawn(ioc, [this]()->asio::awaitable<void> {
		auto d = std::make_shared<Client>(ioc);
		// 如果没连上，就反复的连     // todo:退出机制?
		while (d->stoped) {
			auto r = co_await d->Connect(asio::ip::address::from_string("127.0.0.1"), 55551);
			std::cout << "client Connect r = " << r << std::endl;
		}
		// 连上了，发点啥
		d->Send("asdf\r\n");

		std::cout << "client send asdf" << std::endl;

		// 等一段时间退出
		co_await xx::Timeout(60s);
		}, asio::detached);
}

int main() {
	MyServer server;
	uint16_t port = 55551;
	server.Listen<MyPeer>(port);
	std::cout << "simple telnet server running... port = " << port << std::endl;
	server.ioc.run();
	return 0;
}















//std::vector<std::string_view> output;
//size_t first = 0;
//while (first < strv.size()) {
//	const auto second = strv.find_first_of(delims, first);
//	if (first != second) output.emplace_back(strv.substr(first, second - first));
//	if (second == std::string_view::npos) break;
//	first = second + 1;
//}
//return output;

//size_t peerAutoId = 0;
//std::unordered_map<size_t, std::shared_ptr<PeerBase>> peers;
// size_t id;
//assert(server.peers.contains(id));
// 可能触发析构, 写在最后面, 避免访问成员失效
//server.peers.erase(id);

//std::string addr;	// for log
		//auto ep = socket.remote_endpoint();
		//addr = ep.address().to_string() + ":" + std::to_string(ep.port());
		//std::cout << addr << " accepted." << std::endl;



// asio c++20 lobby
// 总体架构为 单线程, 和 gateway 1 对 多

// asio c++20 gateway
// 总体架构为 伪多线程（ 独立线程负责 accept, socket 均匀分布到多个线程，每个线程逻辑完整独立，互相不通信，自己连别的服务，不共享 )
// 和 lobby 多对 1, 和 client 1 对 多


