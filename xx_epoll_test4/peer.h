#pragma once
#include "server.h"

struct Peer;

// 带超时的回调
struct PeerCB : EP::Timer {
    using Func = std::function<void(xx::ObjBase_s&& o)>;

    // 所在 peer( 移除的时候要用到 )
    Peer *peer;

    // 序号( 移除的时候要用到 )
    int serial = 0;

    // 回调函数本体
    Func func;

    // 继承构造函数
    PeerCB(Peer *const &vpeer, int const &serial, Func &&cbFunc, double const &timeoutSeconds);

    // 执行 func(0,0) 后 从容器移除, 并延迟 Unhold
    void Timeout() override;
};

struct Peer : EP::TcpPeer {
    using EP::TcpPeer::TcpPeer;

    // 循环自增用于生成 serial
    int autoIncSerial = 0;

    // 所有 带超时的回调. key: serial
    std::unordered_map<int, xx::Shared<PeerCB>> callbacks;

    // cleanup callbacks, DelayUnhold
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到数据. 切割后进一步调用 ReceiveXxxxxxx
    void Receive() override;

    // 收到回应( 自动调用 发送请求时设置的回调函数 )
    void ReceiveResponse(int const &serial, xx::ObjBase_s &&o);

    // 收到推送( serial == 0 ), 需要自拟业务逻辑
    void ReceivePush(xx::ObjBase_s &&o);

    // 收到请求( serial 收到时为负数, 但传递到这里时已反转为正数 ), 需要自拟业务逻辑
    void ReceiveRequest(int const &serial, xx::ObjBase_s &&o);


    // return cached instance for quickly send package
    template<typename T>
    xx::Shared<T> const& InstanceOf() const {
        return ((Server*)ec)->om.InstanceOf<T>();
    }

    // 发推送
    int SendPush(xx::ObjBase_s const &o);

    // 发回应
    int SendResponse(int const &serial, xx::ObjBase_s const &o);

    // 发请求（收到相应回应时会触发 cb 执行。超时或断开也会触发，o == nullptr）
    int SendRequest(xx::ObjBase_s const &o, std::function<void(xx::ObjBase_s&& o)> &&cbfunc, double const &timeoutSeconds);
};
