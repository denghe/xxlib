#pragma once
#include "peer.h"
namespace EP = xx::Epoll;

struct CPeer;

// 拨号到服务器 产生的 peer
struct SPeer : Peer {
    // 内部服务编号, 从配置填充
    uint32_t serverId = 0xFFFFFFFFu;

    // 等待对方回 ping 的标志位
    bool waitingPingBack = false;

    // 最后一次发起 ping 的时间
    int64_t lastSendPingMS = 0;

    // 收到回复后计算出来的 ping 值
    int64_t pingMS = 0;

    // 继承构造函数
    using Peer::Peer;

    // 关闭 fd, 从所有 client peers 里的白名单中移除 并下发相应 close. 注册延迟自杀函数( 直接析构并不会触发这个 Close )
    bool Close(int const& reason, std::string_view const& desc) override;

    // 收到正常包
    void ReceivePackage(uint8_t* const& buf, size_t const& len) override;

    // 收到内部指令
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;

    // 从 server cps 容器查找并返回 client peer 的指针. 没找到则返回 空
    CPeer* TryGetCPeer(uint32_t const& clientId);
};
