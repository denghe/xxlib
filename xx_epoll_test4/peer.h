#pragma once
#include "xx_epoll.h"
namespace EP = xx::Epoll;

// 预声明
struct Server;

// 继承 默认 连接覆盖收包函数
struct Peer : EP::TcpPeer {
    // 继承构造函数
    using EP::TcpPeer::TcpPeer;

    // 收到数据. 切割后进一步调用 ReceivePackage 和 ReceiveCommand
    void Receive() override;

    // 收到正常包
    virtual void ReceivePackage(uint32_t const& addr, uint8_t* const& buf, size_t const& len) = 0;

    // 收到内部指令
    virtual void ReceiveCommand(uint8_t* const& buf, size_t const& len) = 0;

    // 构造内部指令包. LEN + ADDR + cmd string + args...
    template<typename...Args>
    void SendTo(uint32_t const& addr, Args const &... cmdAndArgs) {
        xx::Data d;
        d.Reserve(32);
        auto bak = d.WriteJump(sizeof(uint32_t));
        d.WriteFixed(addr);
        d.Write(cmdAndArgs...);
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        this->Send(std::move(d));
    }
};
