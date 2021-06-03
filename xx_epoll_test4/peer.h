﻿#pragma once

#include "server.h"
#include "db.h"
#include "xx_epoll_omhelpers.h"
#include "xx_coro.h"

struct Peer : EP::TcpPeer, EP::OMExt<Peer> {
    using EP::TcpPeer::TcpPeer;

    // cleanup callbacks, DelayUnhold
    bool Close(int const &reason, std::string_view const &desc) override;

    // 发回应 for EP::OMExt<ThisType>
    template<typename PKG = xx::ObjBase, typename ... Args>
    void SendResponse(int32_t const &serial, Args const &... args) {
        if (!Alive()) return;

        // 准备发包填充容器
        xx::Data d(16384);
        // 跳过包头
        auto bak = d.WriteJump<false>(sizeof(uint32_t));
        // 写序号
        d.WriteVarInteger<false>(serial);
        // write args
        if constexpr(std::is_same_v<xx::ObjBase, PKG>) {
            ((Server *) ec)->om.WriteTo(d, args...);
        } else if constexpr(std::is_same_v<xx::Span, PKG>) {
            d.WriteBufSpans(args...);
        } else {
            PKG::WriteTo(d, args...);
        }
        // 填包头
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        // 发包并返回
        Send(std::move(d));
    }

    // 收到数据. 切割后进一步调用 ReceiveXxxxxxx
    void Receive() override;

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&ob);

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int32_t const &serial, xx::ObjBase_s &&ob);


    /***********************************************************************************************/
    // coroutines

    xx::Coros coros;

    template<typename Rtv, typename Func>
    int NewTask(Rtv &rtv, Func &&func) {
        auto serial = GenSerial();
        ((Server *) ec)->db->tp.Add([s = ((Server *) ec), w = xx::SharedFromThis(this).ToWeak(), serial, &rtv, func = std::forward<Func>(func)](DB::Env &env) mutable {
            auto result = func(env);
            s->Dispatch([w = std::move(w), serial, &rtv, result = std::move(result), func = std::move(func)]() mutable {
                if (auto p = w.Lock()) {
                    rtv = std::move(result);
                    p->coros.FireEvent(serial);
                }
            });
        });
        return serial;
    }

    xx::Coro HandleRequest_GetAccountInfoByUsernamePassword(int32_t serial, xx::ObjBase_s ob);
};
