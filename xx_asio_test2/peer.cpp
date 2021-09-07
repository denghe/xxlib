#include "peer.h"
#include "server.h"

void Peer::Receive() {
    // 取出指针备用
    auto buf = recv.buf;
    auto end = recv.buf + recv.len;
    uint32_t dataLen = 0;
    uint32_t addr = 0;

    // 确保包头长度充足
    while (buf + sizeof(dataLen) <= end) {
        // 取长度
        dataLen = *(uint32_t *) buf;

        // 长度异常则断线退出
        if (dataLen < 1 || dataLen > 1024 * 256) {
            Close(-__LINE__, " Peer Receive if (dataLen < 1 || dataLen > 1024 * 256)");
            return;
        }

        // 数据未接收完 就 跳出
        if (buf + sizeof(dataLen) + dataLen > end) break;

        // 跳到数据区开始调用处理回调
        buf += sizeof(dataLen);
        {
            // todo: unpack buf + dataLen, store to recvs
            xx::CoutN("recv data. len = ", dataLen);

            // 如果当前类实例 fd 已 close 则退出
            if (!Alive()) return;
        }
        // 跳到下一个包的开头
        buf += dataLen;
    }

    // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
    recv.RemoveFront(buf - recv.buf);
}

bool Peer::Close(int const& reason, std::string_view const& desc) {
    // 防重入( 同时关闭 fd )
    if (!this->BaseType::Close(reason, desc)) return false;
    LOG_INFO("ip = ", addr," reason = ", reason, ", desc = ", desc);
    // 从容器移除( 减持 )
    ((Server *)ec)->ps.erase(clientId);
    // 延迟减持
    DelayUnhold();
    return true;
}
