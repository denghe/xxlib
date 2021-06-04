#pragma once

#include "server.h"
#include "xx_epoll_omhelpers.h"
#include "pkg_db_service.h"
#include "gpeer.h"
#include "xx_coro.h"
#include "dbpeer.h"

// 虚拟 peer
struct VPeer : EP::Timer, EP::OMExt<VPeer> {
    explicit VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId, std::string &&ip);

    // index at server->vps( fill after create )
    int serverVpsIndex = -1;

    // 指向 gateway peer
    GPeer *gatewayPeer;

    // 存放位于 gateway 的 client id
    uint32_t clientId;

    // client ip address( from cmd: accept )
    std::string ip;

    // logic data
    Database::AccountInfo info;

    /****************************************************************************************/

    // coroutine support
    xx::Coros coros;

    template<typename PKG = xx::ObjBase, typename Rtv, typename ... Args>
    int DbCoroSendRequest(Rtv &rtv, Args const &... args) {
        assert(((Server*)ec)->dbPeer && ((Server*)ec)->dbPeer->Alive());
        return ((Server*)ec)->dbPeer->SendRequest<PKG>([this, &rtv](int32_t const &serial_, xx::ObjBase_s &&ob) {
            rtv = std::move(ob);
            coros.FireEvent(serial_);
        }, 99.0, args...);
    }
	
	// todo: LobbyCoroSendRequest

    //bool flag_Auth = false;
    //xx::Coro HandleRequest_Auth(int serial, xx::ObjBase_s ob);

    /****************************************************************************************/

    // 发回应 for EP::OMExt<ThisType>
    template<typename PKG = xx::ObjBase, typename ... Args>
    void SendResponse(int32_t const &serial, Args const &... args) {
        if (!Alive()) return;

        // 准备发包填充容器
        xx::Data d(16384);
        // 跳过包头
        d.len = sizeof(uint32_t);
        // 写 要发给谁
        d.WriteFixed(clientId);
        // 写序号
        d.WriteVarInteger(serial);
        // write args
        if constexpr(std::is_same_v<xx::ObjBase, PKG>) {
            ((Server *) ec)->om.WriteTo(d, args...);
        } else if constexpr(std::is_same_v<xx::Span, PKG>) {
            d.WriteBufSpans(args...);
        } else {
            PKG::WriteTo(d, args...);
        }
        // 填包头
        *(uint32_t *) d.buf = (uint32_t) (d.len - sizeof(uint32_t));
        // 发包并返回
        gatewayPeer->Send(std::move(d));
    }

    // 收到数据( 进一步解析 serial, unpack 并转发到下面几个函数 )
    void Receive(uint8_t const *const &buf, size_t const &len);

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&o);

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int const &serial, xx::ObjBase_s &&o);

    /****************************************************************************************/

    // swap server->vps.ValueAt( serverVpsIndex & idx )'s network ctx
    [[maybe_unused]] void SwapWith(int const &idx);

    // gatewayPeer != nullptr
    bool Alive() const;

    // kick, remove from server.vps, DelayUnhold
    bool Close(int const &reason, std::string_view const &desc) override;

    // kick, cleanup, update key at server.vps, remove from gpeer.clientIds
    void Kick(int const &reason, std::string_view const &desc, bool const &fromGPeerClose = false);


    /****************************************************************************************/

    // logic here
    void Timeout() override;

    // logic update here
    void Update(double const &dt);
};
