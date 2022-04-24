// asio c++20 lobby
// 总体架构为 单线程, 和 gateway 1 对 多

#include <asio.hpp>
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
using namespace std::literals;
using namespace std::literals::chrono_literals;

/***********************************************************************************************************/
// PeerBase

struct PeerBase : asio::noncopyable {
	virtual ~PeerBase() {}
};
typedef std::shared_ptr<PeerBase> PeerBase_p;

/***********************************************************************************************************/
// Server

struct Server : asio::noncopyable {
	asio::io_context ioc;
	asio::signal_set signals;

	size_t peerAutoId = 0;
	std::unordered_map<size_t, std::shared_ptr<PeerBase>> peers;

	Server()
		: ioc(1)
		, signals(ioc, SIGINT, SIGTERM)
	{
	}

	void Run() {
		ioc.run();
	}

	template<typename PeerType, class = std::enable_if_t<std::is_base_of_v<PeerBase, PeerType>>>
	void Listen(uint16_t port) {
		asio::co_spawn(ioc, ListenCore<PeerType>(port), asio::detached);
	}

protected:
	template<typename PeerType, class = std::enable_if_t<std::is_base_of_v<PeerBase, PeerType>>>
	asio::awaitable<void> ListenCore(uint16_t port) {
		asio::ip::tcp::acceptor acceptor(ioc, { asio::ip::tcp::v6(), port });	// require IP_V6ONLY == false
		for (;;) {
			try {
				asio::ip::tcp::socket socket(ioc);
				co_await acceptor.async_accept(socket, asio::use_awaitable);
				std::make_shared<PeerType>((decltype(PeerType::server))*this, std::move(socket))->Start();
			}
			catch (std::exception& e) {
				std::cout << "Server.Listen() Exception : " << e.what() << std::endl;
			}
		}
	}
};

/***********************************************************************************************************/
// Peer

template<typename ServerType>
struct Peer : PeerBase, std::enable_shared_from_this<Peer<ServerType>> {
	ServerType& server;

	size_t id;
	asio::ip::tcp::socket socket;
	asio::steady_timer timeouter;
	asio::steady_timer writeBarrier;
	std::deque<std::string> writeQueue;

	std::string addr;
	bool stoping = false;
	bool stoped = false;

	Peer(ServerType& server_, asio::ip::tcp::socket&& socket_)
		: server(server_)
		, id(++server_.peerAutoId)
		, socket(std::move(socket_))
		, writeBarrier(server_.ioc)
		, timeouter(server_.ioc)
	{
		writeBarrier.expires_at(std::chrono::steady_clock::time_point::max());
		ResetTimeout(20s);
	}

	// 由于 shared_from_this 无法在构造函数中使用，故拆分出来。平时不可以调用
	void Start() {
		server.peers.insert(std::make_pair(id, this->shared_from_this()));	// 容器持有

		auto ep = socket.remote_endpoint();
		addr = ep.address().to_string() + ":" + std::to_string(ep.port());
		std::cout << addr << " accepted." << std::endl;

		// 启动读写协程( 顺便持有. 协程退出时可能触发析构 )
		asio::co_spawn(server.ioc, [self = this->shared_from_this()]{ return self->Read(); }, asio::detached);
		asio::co_spawn(server.ioc, [self = this->shared_from_this()]{ return self->Write(); }, asio::detached);
	}

	virtual void StopEx() {}

	void Stop() {
		if (stoped) return;
		stoped = true;

		socket.close();
		writeBarrier.cancel();
		timeouter.cancel();

		StopEx();

		assert(server.peers.contains(id));

		// 可能触发析构, 写在最后面, 避免访问成员失效
		server.peers.erase(id);
	}

	void DelayStop(std::chrono::steady_clock::duration const& d) {
		if (stoping || stoped) return;
		ResetTimeout(d);
		stoping = true;
		std::cout << addr << " stoping..." << std::endl;
	}

	void ResetTimeout(std::chrono::steady_clock::duration const& d) {
		timeouter.expires_at(std::chrono::steady_clock::now() + d);
		timeouter.async_wait([this](auto&& ec) {
			if (ec) return;
			Stop();
			});
	}

	void Send(std::string&& msg) {
		if (stoped) return;
		writeQueue.push_back(std::move(msg));
		writeBarrier.cancel_one();
	}

	void Send(std::string const& msg) {
		if (stoped) return;
		writeQueue.push_back(msg);
		writeBarrier.cancel_one();
	}

	virtual int HandleMessage(std::string_view const& msg) = 0;

protected:
	asio::awaitable<void> Read() {
		try {
			for (std::string buf;;) {
				auto n = co_await asio::async_read_until(socket, asio::dynamic_buffer(buf, 1024), "\n", asio::use_awaitable);
				if (stoping || stoped) break;
				if (n > 1) {
					if (auto siz = n - (buf[n - 2] == '\r' ? 2 : 1)) {
						std::string_view sv(buf.data(), siz);
						if (int r = HandleMessage(sv)) {
							std::cout << addr << " HandleMessage: " << sv << ", r = " << r << std::endl;
							break;
						}
					}
				}
				buf.erase(0, n);
			}
		}
		catch (std::exception& ec) {
			std::cout << addr << " Read() ec = " << ec.what() << std::endl;
			Stop();
		}
		std::cout << addr << " Read() end." << std::endl;
	}

	asio::awaitable<void> Write() {
		try {
			while (socket.is_open()) {
				if (writeQueue.empty()) {
					asio::error_code ec;
					co_await writeBarrier.async_wait(asio::redirect_error(asio::use_awaitable, ec));
				}
				else {
					co_await asio::async_write(socket, asio::buffer(writeQueue.front()), asio::use_awaitable);
					writeQueue.pop_front();
				}
				if (stoped) break;
			}
		}
		catch (std::exception& ec) {
			std::cout << addr << " Write() ec = " << ec.what() << std::endl;
			Stop();
		}
		std::cout << addr << " Write() end." << std::endl;
	}
};

/***********************************************************************************************************/
/* logic
* 简单模拟一个 mud 服务器. 连上后，如果 ?? 秒内不发送 有效数据，就 踢掉
* 在 telnet 中，help 回车 可收到帮助
* 有效数据有 2 两种：
* 	1. 指令: 单词 + 空格 + 参数...
* 	2. 回应: 数字 + 空格 + 回应内容...
*/

#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);


struct MyPeer;
struct MyServer : Server {

	// 指令处理函数集合
	std::unordered_map<std::string, std::function<int(MyPeer& p, std::string_view const& args)>> msgHandlers;

	// 游戏循环参与者集合
	std::unordered_set<std::shared_ptr<MyPeer>> broadcasts;

	MyServer();
};
using MyServerPeer = Peer<MyServer>;

struct MyPeer : MyServerPeer {
	using MyServerPeer::MyServerPeer;

	// 便于拿到派生类型指针
	std::shared_ptr<MyPeer> SharedFromThis() {
		return std::static_pointer_cast<MyPeer>(shared_from_this());
	}

	// 业务自增序号
	uint32_t autoSerial = 0;

	// 客户端的回应 临时容器. 发起请求时先插入 自增序号, 阻塞器到此. 收到回应时, 写入 string 并令 timer 解除阻塞
	std::unordered_map<int, std::pair<asio::steady_timer, std::string>> reqs;

	// 创建一个 req 上下文并返回 iter. 顺便重置一下超时
	auto CreateReqs() {
		++autoSerial;
		auto r = reqs.emplace(autoSerial, std::make_pair(asio::steady_timer(server.ioc), ""));
		assert(r.second);
		r.first->second.first.expires_at(std::chrono::steady_clock::time_point::max());
		return r.first;
	}

	// 扩展一下 Stop 的功能
	virtual void StopEx() override {
		for (auto& kv : reqs) {
			kv.second.first.cancel();
		}
	}

	// 防重入
	bool lockLogin = false;
	bool playing = false;

	// 消息路由( 投递进来的不会是 0 长度 )
	virtual int HandleMessage(std::string_view const& msg) override {

		// 找一下空格，方便切割出首个 word / number
		auto n = msg.find_first_of(' ');

		// 根据第一个字母判断性质
		if (msg[0] >= '0' && msg[0] <= '9') {

			// 如果数字，就走序列号流程
			if (n == std::string_view::npos || n + 1 == msg.size()) {
				Send(std::string("invalid message: only number? ") + std::string(msg) + "\r\n");
				return 0;
			}

			// 数字转为 序号
			int serial;
			auto r = std::from_chars(msg.data(), msg.data() + n, serial);
			if (r.ec == std::errc::invalid_argument || r.ec == std::errc::result_out_of_range) {
				Send(std::string("invalid message: serial( string to int ) failed. ") + std::string(msg) + "\r\n");
				return 0;
			}

			// 判断字典里是否能找到
			auto iter = reqs.find(serial);
			if (iter == reqs.end()) {
				Send(std::string("invalid message: serial = ") + std::to_string(serial) + " can't find. " + std::string(msg) + "\r\n");
				return 0;
			}

			// 存储 内容 并放行协程
			iter->second.second = msg.substr(n + 1);
			iter->second.first.cancel();
		}
		else {
			// 单词, 走指令流程. 找到处理函数就 call
			auto word = std::string(msg.substr(0, n));
			if (auto iter = server.msgHandlers.find(word); iter != server.msgHandlers.end()) return iter->second(*this, msg.substr(n + 1));
			Send(std::string("invalid message: word = ") + word + " can't find. " + std::string(msg) + "\r\n");
		}
		return 0;
	}
};

template<class F>
auto MakeScopeGuard(F&& f) noexcept {
	struct ScopeGuard {
		F f; bool cancel;
		explicit ScopeGuard(F&& f) noexcept : f(std::move(f)), cancel(false) {}
		~ScopeGuard() noexcept { if (!cancel) { f(); } }
		inline void Cancel() noexcept { cancel = true; }
		inline void operator()(bool cancel = false) { f(); this->cancel = cancel; }
	};
	return ScopeGuard(std::forward<F>(f));
}

MyServer::MyServer() {
	msgHandlers.insert({ "help", [this](MyPeer& p, std::string_view const& msg) {
		p.Send("avaliable commands: help, login, stop( stop play ), quit( delaystop 3s ), exit( direct stop ).\r\n");
		p.ResetTimeout(20s);
		return 0;
	} });

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
		asio::co_spawn(ioc, [this, p = p.SharedFromThis()]()->asio::awaitable<void> {

			// 退出函数自动解锁
			auto sgLock = MakeScopeGuard([&] { p->lockLogin = false; });

			// 对客户端发起请求
			auto iter = p->CreateReqs();

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
			broadcasts.insert(p);

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
			broadcasts.erase(std::static_pointer_cast<MyPeer>(p.shared_from_this()));
		}
		return 0;
	} });

	msgHandlers.insert({ "quit", [this](MyPeer& p, std::string_view const& args) {
		p.Send("bye!\r\n");
		if (p.playing) {
			p.playing = false;
			broadcasts.erase(std::static_pointer_cast<MyPeer>(p.shared_from_this()));
		}
		p.DelayStop(3s);
		return 0;
	} });

	msgHandlers.insert({ "exit", [this](MyPeer& p, std::string_view const& args) {
		p.Stop();
		return 1;
	} });

	// 模拟一个固定帧率的游戏循环。玩家 peer 如果纳入 broadcasts 就能收到 "广播". 如果玩家还有别的上下文信息，可以和 peer 双向关联
	asio::co_spawn(ioc, [this]()->asio::awaitable<void> {
		int frameNumber = 0;
		asio::steady_timer delay(ioc);
		while (true) {
			++frameNumber;
			auto iter = broadcasts.begin();
			while (iter != broadcasts.end()) {
				if (!(*iter)->stoped) {
					(*iter)->Send(std::to_string(frameNumber) + "\r");
					++iter;
				}
				else {
					iter = broadcasts.erase(iter);
				}
			}
			delay.expires_after(10ms); co_await delay.async_wait(use_nothrow_awaitable);	// yield 10ms
		}
	}, asio::detached);
}

int main() {
	try {
		MyServer server;
		server.Listen<MyPeer>(55551);
		server.Run();
	}
	catch (std::exception& e) {
		std::cout << "main() Exception : " << e.what() << std::endl;
		return -1;
	}
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
