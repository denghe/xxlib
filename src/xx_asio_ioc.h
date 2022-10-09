#pragma once

#include <xx_data_shared.h>
#include <xx_ptr.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
using asio::co_spawn;
using asio::awaitable;
using asio::detached;

namespace xx {
	// 协程内使用的 通用超时函数, 实现 sleep 的效果
	inline awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
		asio::steady_timer t(co_await asio::this_coro::executor);
		t.expires_after(d);
		co_await t.async_wait(use_nothrow_awaitable);
	}


	// 可用于 服务主体 或 worker 继承使用，以方便各处传递调用. 需确保生命周期最长
	struct IOCBase : asio::io_context {
		using asio::io_context::io_context;

		// 开始监听一个 tcp 端口
		template<typename SocketHandler>
		void Listen(uint16_t const& port, SocketHandler&& sh) {
			co_spawn(*this, [this, port, sh = std::move(sh)]()->awaitable<void> {
				asio::ip::tcp::acceptor acceptor(*this, { asio::ip::tcp::v6(), port });
				acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
				for (;;) {
					asio::ip::tcp::socket socket(*this);
					if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
						sh(std::move(socket));
					}
					else break;
				}
			}, detached);
		}
	};
}

// 用于 template<typename PeerDeriveType, ....> struct XxxxxxxxCode 内部强转 this 为 派生类型
// 当前所有 模板代码片段 继承规则：最终派生类用 PeerDeriveType 类型名存储，用下面的宏来访问
#define PEERTHIS ((PeerDeriveType*)(this))
