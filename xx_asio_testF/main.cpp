#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/process.hpp>
#include <boost/process/async.hpp>
#include <iomanip>
#include <iostream>

void handle_batch(std::vector<size_t> params) {
    static std::mutex s_mx;

    if (!params.empty()) {
        // emulate some work, because I'm lazy
        auto sum = std::accumulate(begin(params), end(params), 0ull);
        // then wait some 100..200ms
        {
            using namespace std::chrono_literals;
            std::mt19937 prng(std::random_device{}());
            std::this_thread::sleep_for(
                std::uniform_real_distribution<>(100, 200)(prng) * 1ms);
        }

        // simple thread id (thread::id displays ugly)
        auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id()) % 100;

        // report results to stdout
        std::lock_guard lk(s_mx); // make sure the output doesn't intermix
        std::cout
            << "Thread #" << std::setw(2) << std::setfill('0') << tid
            << " Batch n:" << params.size()
            << "\tRange [" << params.front() << ".." << params.back() << "]"
            << "\tSum:" << sum
            << std::endl;
    }
}

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace process = boost::process;

using boost::system::error_code;
using boost::system::system_error;
using net::ip::tcp;
using stream = ssl::stream<tcp::socket>;

auto ssl_context() {
    ssl::context ctx{ ssl::context::sslv23 };
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_peer);
    return ctx;
}

void connect_https(stream& s, std::string const& host, tcp::resolver::iterator eps) {
    net::connect(s.lowest_layer(), eps);
    s.lowest_layer().set_option(tcp::no_delay(true));

    if (!SSL_set_tlsext_host_name(s.native_handle(), host.c_str())) {
        throw system_error{ { (int)::ERR_get_error(), net::error::get_ssl_category() } };
    }
    s.handshake(stream::handshake_type::client);
}

auto get_request(std::string const& host, std::string const& path) {
    using namespace http;
    request<string_body> req;
    req.version(11);
    req.method(verb::get);
    req.target("https://" + host + path);
    req.set(field::user_agent, "test");
    req.set(field::host, host);

    std::cerr << req << std::endl;
    return req;
}

int main() {
    net::io_context io; // main thread does all io
    net::thread_pool pool(6); // worker threads

    // outside for lifetime
    http::response_parser<http::buffer_body> response_reader;
    beast::flat_buffer lookahead; // for the response_reader
    std::array<char, 512> buf{ 0 }; // for download content
    auto ctx = ssl_context();
    ssl::stream<tcp::socket> s(io, ctx);

    {   // synchronously write request
        std::string host = "www.mathsisfun.com";
        connect_https(s, host, tcp::resolver{ io }.resolve(host, "https"));
        http::write(s, get_request(host, "/includes/primes-to-100k.zip"));

        http::read_header(s, lookahead, response_reader);
        //std::cerr << "Headers: " << response_reader.get().base() << std::endl;
    }

    // now, asynchoronusly read contents
    process::async_pipe pipe_to_zcat(io);

    std::function<void(error_code, size_t)> receive_zip;
    receive_zip = [&s, &response_reader, &pipe_to_zcat, &buf, &lookahead, &receive_zip](error_code ec, size_t /*ignore_this*/) {
        auto& res = response_reader.get();
        auto& body = res.body();
        if (body.data) {
            auto n = sizeof(buf) - body.size;
            net::write(pipe_to_zcat, net::buffer(buf, n));
        }

        bool done = ec && !(ec == http::error::need_buffer);
        done += response_reader.is_done();

        if (done) {
            std::cerr << "receive_zip: " << ec.message() << std::endl;
            pipe_to_zcat.close();
        } else {
            body.data = buf.data();
            body.size = buf.size();

            http::async_read(s, lookahead, response_reader, receive_zip);
        }
    };

    // kick off receive loop
    receive_zip(error_code{}, 0);

    process::async_pipe zcat_output(io);
    process::child zcat(
        process::search_path("zcat"),
        process::std_in < pipe_to_zcat,
        process::std_out > zcat_output,
        process::on_exit([](int exitcode, std::error_code ec) {
            std::cerr << "Child process exited with " << exitcode << " (" << ec.message() << ")\n";
            }), io);

    std::function<void(error_code, size_t)> receive_primes;
    net::streambuf sb;
    receive_primes = [&zcat_output, &sb, &receive_primes, &pool](error_code ec, size_t /*transferred*/) {
        {
            std::istream is(&sb);

            size_t n = std::count(net::buffers_begin(sb.data()), net::buffers_end(sb.data()), '\n');
            std::vector<size_t> batch(n);
            std::copy_n(std::istream_iterator<size_t>(is), n, batch.begin());
            is.ignore(1, '\n'); // we know a newline is pending, eat it to keep invariant

            post(pool, std::bind(handle_batch, std::move(batch)));
        }

        if (ec) {
            std::cerr << "receive_primes: " << ec.message() << std::endl;
            zcat_output.close();
        } else {
            net::async_read_until(zcat_output, sb, "\n", receive_primes);
        }
    };
    // kick off handler loop as well:
    receive_primes(error_code{}, 0);

    io.run();
    pool.join();
}
