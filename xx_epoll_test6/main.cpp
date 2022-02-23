#include "xx_signal.h"
#define XX_DISABLE_READLINE
#include "xx_epoll.h"
#include "xx_logger.h"

namespace EP = xx::Epoll;

int64_t counter = 0;

struct Peer : EP::TcpPeer {
    using BaseType = EP::TcpPeer;
    using BaseType::BaseType;

    void Receive() override {
        Send(recv.buf, recv.len);   // echo
        recv.Clear();
        ++counter;
    }

    bool Close(int const &reason, std::string_view const &desc) override {
        if (!this->BaseType::Close(reason, desc)) return false;
        DelayUnhold();
        LOG_INFO("Close ip = ", addr);
        return true;
    }
};

struct Listener : EP::TcpListener<Peer> {
    using EP::TcpListener<Peer>::TcpListener;

    void Accept(xx::Shared<Peer> const &peer) override {
        peer->Hold();
        LOG_INFO("accept ip = ", peer->addr);
    }
};

struct Timer : EP::Timer {
    explicit Timer(EP::Context* const &ec) : EP::Timer(ec) {
        SetTimeoutSeconds(1);
    }
    void Timeout() override {
        std::cout << counter << std::endl;
        counter = 0;
        SetTimeoutSeconds(1);
    }
};

struct Server : EP::Context {
    using EP::Context::Context;
    xx::Shared<Listener> listener;
    xx::Shared<Timer> timer;
    int Init() {
        listener.Emplace(this);
        if (int r = listener->Listen(55555)) {
            LOG_ERROR("listen to port ", 55555, "failed.");
            return r;
        }
        timer.Emplace(this);
        return 0;
    }
};

int main() {
    xx::IgnoreSignal();
    Server s;
    if (int r = s.Init()) return r;
    return s.Run();
}
