#pragma once
#include "server.h"
#include "xx_epoll_omhelpers.h"

struct Peer : EP::TcpPeer, EP::OMExt<Peer> {
    using EP::TcpPeer::TcpPeer;

    // cleanup callbacks, DelayUnhold
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到数据. 切割后进一步调用 ReceiveXxxxxxx
    void Receive() override;

    // 发回应 for EP::OMExt<ThisType>
    template<typename PKG = xx::ObjBase, typename ... Args>
    int SendResponse(int32_t const &serial, Args const& ... args) {
        if (!Alive()) return __LINE__;

        // 准备发包填充容器
        xx::Data d(16384);
        // 跳过包头
        auto bak = d.WriteJump(sizeof(uint32_t));
        // 写序号
        d.WriteVarInteger(serial);
        // write args
        if constexpr(std::is_same_v<xx::ObjBase, PKG>) {
            ((Server*)ec)->om.WriteTo(d, args...);
        }
        else if constexpr(std::is_same_v<xx::Span, PKG>) {
            d.WriteBufSpans(args...);
        }
        else {
            PKG::WriteTo(d, args...);
        }
        // 填包头
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        // 发包并返回
        return Send(std::move(d));
    }

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    virtual void ReceivePush(xx::ObjBase_s &&ob) = 0;

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    virtual void ReceiveRequest(int const &serial, xx::ObjBase_s &&ob) = 0;
};
