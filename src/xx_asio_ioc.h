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
	// sleep for coroutine
	inline awaitable<void> Timeout(std::chrono::steady_clock::duration d) {
		asio::steady_timer t(co_await asio::this_coro::executor);
		t.expires_after(d);
		co_await t.async_wait(use_nothrow_awaitable);
	}


	// server or worker's base class. need to ensure the longest life cycle
	struct IOCBase : asio::io_context {
		using asio::io_context::io_context;

		// start listen an tcp port
		template<typename SocketHandler>
		void Listen(uint16_t const& port, SocketHandler&& sh) {
			co_spawn(*this, [this, port, sh = std::move(sh)]()->awaitable<void> {
				asio::ip::tcp::acceptor acceptor(*this, { asio::ip::tcp::v6(), port });
				acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
				while (!stopped()) {
					asio::ip::tcp::socket socket(*this);
					if (auto [ec] = co_await acceptor.async_accept(socket, use_nothrow_awaitable); !ec) {
						sh(std::move(socket));
					}
					else break;
				}
			}, detached);
		}
	};

	template<typename EndPoint = asio::ip::tcp::endpoint, typename S>
	EndPoint AddressToEndpoint(S&& ipstr, asio::ip::port_type port) {
		return EndPoint(asio::ip::address::from_string(ipstr), port);
	}
}

// used to force convert "this" to derived type
#define PEERTHIS ((PeerDeriveType*)(this))

// member func exists checkers
template<typename T> concept Has_Start_ = requires(T t) { t.Start_(); };
template<typename T> concept Has_Stop_ = requires(T t) { t.Stop_(); };
