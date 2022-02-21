//
// refactored_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <cstdio>

using asio::ip::tcp;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
namespace this_coro = asio::this_coro;

using namespace std::chrono_literals;

awaitable<void> echo_once(tcp::socket& socket)
{
    char data[128];
    asio::steady_timer timer(socket.get_executor(), std::chrono::steady_clock::now() + 5s);
    timer.async_wait([&](auto e) { if (!e) socket.close(e); });
    std::size_t n = co_await socket.async_read_some(asio::buffer(data), use_awaitable);
    timer.cancel();
    co_await async_write(socket, asio::buffer(data, n), use_awaitable);
}

awaitable<void> echo(tcp::socket socket)
{
    try
    {
        for (;;)
        {
            // The asynchronous operations to echo a single chunk of data have been
            // refactored into a separate function. When this function is called, the
            // operations are still performed in the context of the current
            // coroutine, and the behaviour is functionally equivalent.
            co_await echo_once(socket);
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

int main()
{

    try
    {
        asio::io_context io_context(10000);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        co_spawn(io_context, listener(), detached);

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::printf("Exception: %s\n", e.what());
    }
}

/*

    awaitable<bool> handshake()
    {
        asio::streambuf streambuf;
        size_t n = co_await asio::async_read_until(source, streambuf, "\r\n", use_awaitable);
        if (0 == n)
            co_return false;

        auto line = std::string_view{ (const char*)streambuf.data().data(), n };
        size_t method_end;
        if ((method_end = line.find(' ')) == std::string_view::npos)
        {
            co_return false;
        }

        auto method = line.substr(0, method_end);
        size_t query_start = std::string_view::npos;
        size_t path_and_query_string_end = std::string_view::npos;
        for (size_t i = method_end + 1; i < line.size(); ++i)
        {
            if (line[i] == '?' && (i + 1) < line.size())
            {
                query_start = i + 1;
            }
            else if (line[i] == ' ')
            {
                path_and_query_string_end = i;
                break;
            }
        }

        std::string_view scheme;
        std::string_view host;
        uint16_t port;
        std::string_view path;
        parse_url(line.substr(method_end + 1, path_and_query_string_end - method_end - 1), scheme, host, port, path);

        asio::ip::tcp::resolver resolver(parent->io_context());
        if (!proxy_config::get().proxy_host.empty())
        {
            auto endpoints = co_await resolver.async_resolve(proxy_config::get().proxy_host, std::to_string(proxy_config::get().proxy_port), use_awaitable);
            co_await asio::async_connect(destination.lowest_layer(), endpoints, use_awaitable);

            std::string host_port_string;
            host_port_string.append(host);
            host_port_string.append(":");
            host_port_string.append(std::to_string(port));

            std::string data;
            data.append("CONNECT ");
            data.append(host_port_string);
            data.append(" HTTP/1.1\r\n");
            data.append("Host: ");
            data.append(host_port_string);
            data.append("\r\n\r\n");

            co_await asio::async_write(destination.next_layer(), asio::buffer(data.data(), data.size()), use_awaitable);

            asio::streambuf readbuf;
            co_await asio::async_read_until(destination.next_layer(), readbuf, "\r\n\r\n", use_awaitable);
            if (0 == n)
                co_return false;

            LOG_INFO("recv proxy response: %s", std::string{ (const char*)readbuf.data().data(), readbuf.size() }.data());
        }
        else
        {
            auto endpoints = co_await resolver.async_resolve(host, std::to_string(port), use_awaitable);
            co_await asio::async_connect(destination.lowest_layer(), endpoints, use_awaitable);
        }

        SSL_set_tlsext_host_name(destination.native_handle(), std::string{host}.data());
        co_await destination.async_handshake(asio::ssl::stream_base::client, use_awaitable);

        if (proxy_config::get().proxy_host.empty())
        {
            std::string str;
            str.append(method);
            str.append(1, ' ');
            str.append(path);
            str.append(1, ' ');
            str.append(line.substr(path_and_query_string_end + 1));
            size_t num_additional_bytes = streambuf.size() - n;
            streambuf.consume(n);
            str.append((const char*)streambuf.data().data(), num_additional_bytes);
            co_await asio::async_write(destination, asio::buffer(str.data(), str.size()), use_awaitable);
        }
        else
        {
            co_await asio::async_write(destination, streambuf, use_awaitable);
        }
        co_return true;
    }



    https://developpaper.com/quick-understanding-boost-asio-multithreading-model-based-on/

*/
