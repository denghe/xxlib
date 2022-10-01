//
// echo_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/read.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdio>
#include <string_view>
#include <chrono>
#include <iostream>

using namespace std::literals;
namespace asio = boost::asio;

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
namespace this_coro = asio::this_coro;

#if defined(ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

awaitable<void> echo(tcp::socket socket)
{
    try
    {
        char data[1024];
        for (;;)
        {
            std::size_t n = co_await socket.async_read_some(asio::buffer(data), use_awaitable);
            co_await async_write(socket, asio::buffer(data, n), use_awaitable);
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }
}

awaitable<void> listener()
{
    auto executor = co_await this_coro::executor;
    tcp::acceptor acceptor(executor, { tcp::v4(), 55555 });
    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(executor, echo(std::move(socket)), detached);
    }
}

static size_t g_counter = 0;

awaitable<void> run_client()
{
    auto executor = co_await this_coro::executor;

    tcp::socket sock(executor);

    tcp::resolver resolver(executor);

    std::string data = "hello";
    std::array<char, 5> buffer;
    co_await sock.async_connect(*resolver.resolve("127.0.0.1", std::to_string(55555)).begin(), use_awaitable);
    for (;;)
    {
        co_await sock.async_send(asio::buffer(data), use_awaitable);
        co_await asio::async_read(sock, asio::buffer(buffer), use_awaitable);
        ++g_counter;
    }
}

awaitable<void> run_timer()
{
    auto executor = co_await this_coro::executor;
    asio::steady_timer timer(executor);
    while (true)
    {
        timer.expires_after(std::chrono::seconds(1));
        co_await timer.async_wait(use_awaitable);
        std::cout << "count: " << g_counter << std::endl;
        g_counter = 0;
    }
}

int main(int argc, char** argv)
{
    try
    {
        std::string mode = "S";

        for (int i = 1; i < argc; ++i)
        {
            bool lastarg = i == (argc - 1);
            std::string_view v{ argv[i] };
            if ((v == "-m"sv || v == "--mode"sv) && !lastarg)
            {
                mode = "C";
            }
        }

        if (mode == "S")
        {
            asio::io_context io_context(1);

            asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) { io_context.stop(); });

            co_spawn(io_context, listener(), detached);

            io_context.run();
        }
        else
        {
            asio::io_context io_context(1);
            asio::signal_set signals(io_context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto) { io_context.stop(); });

            co_spawn(io_context, run_client(), detached);
            co_spawn(io_context, run_timer(), detached);
            io_context.run();
        }
    }
    catch (std::exception& e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}