// asio c++20 gateway

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
#include <chrono>
using namespace std::literals::chrono_literals;
#include <iostream>


asio::awaitable<void> timeout(std::chrono::steady_clock::duration d) {
	asio::steady_timer t(co_await asio::this_coro::executor);
	t.expires_after(d);
	co_await t.async_wait(use_nothrow_awaitable);
}

asio::awaitable<void> logic(asio::io_context& ioc) {
LabBegin:
	{
		std::cout << "begin resolve" << std::endl;

		asio::ip::tcp::resolver resolver(ioc);
		auto r1 = co_await(resolver.async_resolve("127.0.0.1", "55555", use_nothrow_awaitable) || timeout(5s));
		if (r1.index()) {
			std::cout << "resolve timeout" << std::endl;
			goto LabEnd;
		}
		auto& [e1, rs] = std::get<0>(r1);
		if (e1) {
			std::cout << "resolve error = " << e1 << std::endl;
			goto LabEnd;
		}
		else {
			std::cout << "resolve success. ip list = {" << std::endl;
			for (auto const& r : rs) {
				std::cout << "    " << r.endpoint() << std::endl;
			}
			std::cout << "}" << std::endl;
		}

		std::cout << "begin connect" << std::endl;

		// todo: 同时连多个 ip. 保留先连上的

		asio::ip::tcp::socket s(ioc);
		auto r2 = co_await(s.async_connect(*rs.begin(), use_nothrow_awaitable) || timeout(5s));
		if (r2.index()) {
			std::cout << "connect timeout" << std::endl;
			goto LabEnd;
		}
		auto& [e2] = std::get<0>(r2);
		if (e2) {
			std::cout << "connect error = " << e2 << std::endl;
			goto LabEnd;
		}

		std::cout << "begin w + r" << std::endl;

		while (true) {
			auto str = "a";
			auto strLen = 1;
			auto r3 = co_await(asio::async_write(s, asio::buffer(str, strLen), use_nothrow_awaitable) || timeout(5s));
			if (r3.index()) {
				std::cout << "write timeout" << std::endl;
				goto LabEnd;
			}
			auto& [e3, wsiz] = std::get<0>(r3);
			if (e3) {
				std::cout << "write error = " << e3 << std::endl;
				goto LabEnd;
			}
			if (wsiz < strLen) {
				std::cout << "write failed. wsiz = " << wsiz << ", strLen = " << strLen << std::endl;
				goto LabEnd;
			}

			char data;
			auto dataLen = 1;
			auto r4 = co_await(s.async_read_some(asio::buffer(&data, dataLen), use_nothrow_awaitable) || timeout(5s));
			if (r4.index()) {
				std::cout << "read timeout" << std::endl;
				goto LabEnd;
			}
			auto& [e4, rsiz] = std::get<0>(r4);
			if (e4) {
				std::cout << "read error = " << e4 << std::endl;
				goto LabEnd;
			}
			if (rsiz < dataLen) {
				std::cout << "read failed. rsiz = " << rsiz << std::endl;
				goto LabEnd;
			}

			if (str[0] != data) {
				std::cout << "warning: read bad data. str[0] = " << (int)str[0] << ", data = " << (int)data << std::endl;
			}
		}
	}
LabEnd:
	co_await timeout(1s);
	goto LabBegin;
}

double NowMS() {
	return std::chrono::system_clock::now().time_since_epoch().count() / 10000.;
}

int main() {
	asio::io_context ios(1);
	auto&& iosGuard = asio::make_work_guard(ios);

	asio::co_spawn(ios, logic(ios), asio::detached);

	auto ms = NowMS();
	while (true) {
		std::this_thread::sleep_for(500ms);			// 模拟游戏每帧来一发
		std::cout << "------------------------------------------------- frame ms = " << (NowMS() - ms) << std::endl;
		ios.poll_one();
	}
	return 0;
}

























//// 模拟 co_yield 暂停一帧当前协程的执行
//#define YIELD (co_await asio::system_timer(co_await asio::this_coro::executor).async_wait(asio::use_awaitable))



////
//// chat_server.cpp
//// ~~~~~~~~~~~~~~~
////
//// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
////
//// Distributed under the Boost Software License, Version 1.0. (See accompanying
//// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////
//
//#include <cstdlib>
//#include <deque>
//#include <iostream>
//#include <list>
//#include <memory>
//#include <set>
//#include <string>
//#include <utility>
//#include <asio/awaitable.hpp>
//#include <asio/detached.hpp>
//#include <asio/co_spawn.hpp>
//#include <asio/io_context.hpp>
//#include <asio/ip/tcp.hpp>
//#include <asio/read_until.hpp>
//#include <asio/redirect_error.hpp>
//#include <asio/signal_set.hpp>
//#include <asio/steady_timer.hpp>
//#include <asio/use_awaitable.hpp>
//#include <asio/write.hpp>
//
//using asio::ip::tcp;
//using asio::awaitable;
//using asio::co_spawn;
//using asio::detached;
//using asio::redirect_error;
//using asio::use_awaitable;
//
////----------------------------------------------------------------------
//
//class chat_participant
//{
//public:
//    virtual ~chat_participant() {}
//    virtual void deliver(const std::string& msg) = 0;
//};
//
//typedef std::shared_ptr<chat_participant> chat_participant_ptr;
//
////----------------------------------------------------------------------
//
//class chat_room
//{
//public:
//    void join(chat_participant_ptr participant)
//    {
//        participants_.insert(participant);
//        for (auto msg : recent_msgs_)
//            participant->deliver(msg);
//    }
//
//    void leave(chat_participant_ptr participant)
//    {
//        participants_.erase(participant);
//    }
//
//    void deliver(const std::string& msg)
//    {
//        recent_msgs_.push_back(msg);
//        while (recent_msgs_.size() > max_recent_msgs)
//            recent_msgs_.pop_front();
//
//        for (auto participant : participants_)
//            participant->deliver(msg);
//    }
//
//private:
//    std::set<chat_participant_ptr> participants_;
//    enum { max_recent_msgs = 100 };
//    std::deque<std::string> recent_msgs_;
//};
//
////----------------------------------------------------------------------
//
//class chat_session
//    : public chat_participant,
//    public std::enable_shared_from_this<chat_session>
//{
//public:
//    chat_session(tcp::socket socket, chat_room& room)
//        : socket_(std::move(socket)),
//        timer_(socket_.get_executor()),
//        room_(room)
//    {
//        timer_.expires_at(std::chrono::steady_clock::time_point::max());
//    }
//
//    void start()
//    {
//        room_.join(shared_from_this());
//
//        co_spawn(socket_.get_executor(),
//            [self = shared_from_this()]{ return self->reader(); },
//            detached);
//
//        co_spawn(socket_.get_executor(),
//            [self = shared_from_this()]{ return self->writer(); },
//            detached);
//    }
//
//    void deliver(const std::string& msg)
//    {
//        write_msgs_.push_back(msg);
//        timer_.cancel_one();
//    }
//
//private:
//    awaitable<void> reader()
//    {
//        try
//        {
//            for (std::string read_msg;;)
//            {
//                std::size_t n = co_await asio::async_read_until(socket_,
//                    asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);
//
//                room_.deliver(read_msg.substr(0, n));
//                read_msg.erase(0, n);
//            }
//        }
//        catch (std::exception&)
//        {
//            stop();
//        }
//    }
//
//    awaitable<void> writer()
//    {
//        try
//        {
//            while (socket_.is_open())
//            {
//                if (write_msgs_.empty())
//                {
//                    asio::error_code ec;
//                    co_await timer_.async_wait(redirect_error(use_awaitable, ec));
//                }
//                else
//                {
//                    co_await asio::async_write(socket_,
//                        asio::buffer(write_msgs_.front()), use_awaitable);
//                    write_msgs_.pop_front();
//                }
//            }
//        }
//        catch (std::exception&)
//        {
//            stop();
//        }
//    }
//
//    void stop()
//    {
//        room_.leave(shared_from_this());
//        socket_.close();
//        timer_.cancel();
//    }
//
//    tcp::socket socket_;
//    asio::steady_timer timer_;
//    chat_room& room_;
//    std::deque<std::string> write_msgs_;
//};
//
////----------------------------------------------------------------------
//
//awaitable<void> listener(tcp::acceptor acceptor)
//{
//    chat_room room;
//
//    for (;;)
//    {
//        std::make_shared<chat_session>(
//            co_await acceptor.async_accept(use_awaitable),
//            room
//            )->start();
//    }
//}
//
////----------------------------------------------------------------------
//
//int main(int argc, char* argv[])
//{
//    try
//    {
//        if (argc < 2)
//        {
//            std::cerr << "Usage: chat_server <port> [<port> ...]\n";
//            return 1;
//        }
//
//        asio::io_context io_context(1);
//
//        for (int i = 1; i < argc; ++i)
//        {
//            unsigned short port = std::atoi(argv[i]);
//            co_spawn(io_context,
//                listener(tcp::acceptor(io_context, { tcp::v4(), port })),
//                detached);
//        }
//
//        asio::signal_set signals(io_context, SIGINT, SIGTERM);
//        signals.async_wait([&](auto, auto) { io_context.stop(); });
//
//        io_context.run();
//    }
//    catch (std::exception& e)
//    {
//        std::cerr << "Exception: " << e.what() << "\n";
//    }
//
//    return 0;
//}
