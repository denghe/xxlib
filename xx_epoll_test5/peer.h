#pragma once
#include "server.h"
#include "xx_epoll_omhelpers.h"

struct Peer : EP::TcpPeer, EP::OMExt<Peer> {
    using EP::TcpPeer::TcpPeer;

    // cleanup callbacks, DelayUnhold
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到数据. 切割后进一步调用 ReceiveXxxxxxx
    void Receive() override;

    // return cached instance for quickly send package
    template<typename T>
    xx::Shared<T> const& InstanceOf() const {
        return ((Server*)ec)->om.InstanceOf<T>();
    }

    // 发回应
    int SendResponse(int const &serial, xx::ObjBase_s const &ob);

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    virtual void ReceivePush(xx::ObjBase_s &&ob) = 0;

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    virtual void ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) = 0;
};
