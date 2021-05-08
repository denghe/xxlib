#pragma onceBaseType
#include "kpeer.h"
#include "tsl/hopscotch_set.h"

// 客户端 连进来产生的 peer
struct CPeer : KPeer {
	using BaseType = KPeer;
    // 自增编号, accept 时填充
    uint32_t clientId = 0xFFFFFFFFu;

    // 允许访问的 server peers 的 id 的白名单
    tsl::hopscotch_set<uint32_t> serverIds;

    // 继承构造函数
    using BaseType::BaseType;

    // 群发断开指令, 从容器移除变野,  DelayUnhold 自杀
    bool Close(int const& reason, std::string_view const& desc) override;

    // 延迟关闭( 设置 closed = true, 群发断开指令, 从容器移除变野, 靠超时自杀 )
    void DelayClose(double const& delaySeconds);

    // 收到正常包
    void ReceivePackage(uint8_t* const& buf, size_t const& len) override;

    // 收到内部指令
    void ReceiveCommand(uint8_t* const& buf, size_t const& len) override;

protected:
    // Close & DelayClose 的公共部分。群发断开指令 并从容器移除
    void PartialClose();
};
