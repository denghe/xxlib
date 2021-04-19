#pragma once
#include "xx_epoll_kcp.h"
namespace EP = xx::Epoll;

// 预声明
struct Server;

// 继承 默认 连接覆盖收包函数
struct KPeer : EP::KcpPeer {
    // 是否已关闭. true: 拒收数据, 且断线时不再次执行 Dispose  ( 主用于 延迟掐线 )
    bool closed = false;

    // 继承构造函数
    using EP::KcpPeer::KcpPeer;

    // 收到数据. 切割后进一步调用 ReceivePackage 和 ReceiveCommand
    void Receive() override;

    // 收到正常包
    virtual void ReceivePackage(uint8_t* const& buf, size_t const& len) = 0;

    // 收到内部指令
    virtual void ReceiveCommand(uint8_t* const& buf, size_t const& len) = 0;

    // 构造内部指令包. LEN + ADDR + cmd string + args...
    template<typename...Args>
    void SendCommand(Args const &... cmdAndArgs) {
        auto&& d = ec->data;
        d.Clear();
        d.Reserve(32);
        auto bak = d.WriteJump(sizeof(uint32_t));
        d.WriteFixed(0xFFFFFFFFu);
        d.Write(cmdAndArgs...);
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        this->Send(d.buf, d.len);
    }
};
