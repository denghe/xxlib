#pragma once

#include "server.h"
#include "xx_epoll_omhelpers.h"
#include "pkg_db_service.h"

struct GPeer;
struct VPeer;
struct Game;

// 虚拟 peer
struct VPeer : EP::Timer, EP::OMExt<VPeer>, EP::TypeCounterExt {
    explicit VPeer(Server *const &server, GPeer *const &gatewayPeer, uint32_t const &clientId, std::string&& ip);

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
    int gameId = -1;
    int serviceId = -1;

    /****************************************************************************************/
    // helpers

    // return cached instance for quickly Send Push / Response   ( can't hold )
    template<typename T>
    xx::Shared<T> const& InstanceOf() const {
        assert(((Server*)ec)->om.InstanceOf<T>().useCount() == 1);
        return ((Server*)ec)->om.InstanceOf<T>();
    }

    /****************************************************************************************/

    // 发回应
    int SendResponse(int32_t const &serial, xx::ObjBase_s const &ob);

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
    void Kick(int const &reason, std::string_view const &desc, bool const& fromGPeerClose = false);


    /****************************************************************************************/

    // logic here
    void Timeout() override;

    // logic update here
    void Update(double const &dt);

    // accountId < 0
    bool IsGuest() const;

    // guest: set accountId, update key / swap.  return 0: online success. 1: swap success.  error: < 0
    int SetAccount(Database::AccountInfo const& ai);
};
