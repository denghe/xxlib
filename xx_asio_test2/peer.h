#pragma once
#include "xx_obj.h"
#include "xx_epoll_kcp.h"
namespace EP = xx::Epoll;

// 预声明
struct Server;

// 继承 默认 连接覆盖收包函数
struct Peer : EP::KcpPeer {
    using EP::KcpPeer::KcpPeer;
    using BaseType = EP::KcpPeer;

    // 自增编号, accept 时填充
    uint32_t clientId = 0xFFFFFFFFu;

    // received packages
    std::queue<xx::ObjBase_s> recvs;

    // flags
    bool synced = false;

    // 收到数据: unpack & store to recvs
    void Receive() override;

    // 从容器移除变野,  DelayUnhold 自杀
    bool Close(int const& reason, std::string_view const& desc) override;

    // server.WriteTo(o, tmp); Send(tmp)
    void Send(xx::ObjBase_s const& o);
    void Send(xx::Data const& d);
};
