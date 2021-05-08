#include "peer.h"
#include "server.h"

void Peer::Receive() {
    // 如果属于延迟踢人拒收数据状态，直接清数据短路退出
    if (closed) {
        recv.Clear();
        return;
    }

    // 取出指针备用
    auto buf = recv.buf;
    auto end = recv.buf + recv.len;
    uint32_t dataLen = 0;
    uint32_t addr = 0;

    // 确保包头长度充足
    while (buf + sizeof(dataLen) <= end) {
        // 取长度
        dataLen = *(uint32_t *) buf;

        // 长度异常则断线退出( 不含地址? 超长? 256k 不够可以改长 )
        if (dataLen < sizeof(addr) || dataLen > 1024 * 256) {
            Close(-__LINE__, xx::ToString("if (dataLen < sizeof(addr) || dataLen > 1024 * 256), dataLen = ", dataLen));
            return;
        }

        // 数据未接收完 就 跳出
        if (buf + sizeof(dataLen) + dataLen > end) break;

        // 跳到数据区开始调用处理回调
        buf += sizeof(dataLen);
        {
            // 取出地址
            addr = *(uint32_t *)buf;

            // 包类型判断
            if (addr == 0xFFFFFFFFu) {
                // 内部指令. 传参时跳过 addr 部分
                ReceiveCommand(buf + sizeof(addr), dataLen - sizeof(addr));
            } else {
                // 普通包. id 打头
                ReceivePackage(buf, dataLen);
            }

            // 如果当前类实例 fd 已 close 则退出
            if (!Alive() || closed) return;
        }
        // 跳到下一个包的开头
        buf += dataLen;
    }

    // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
    recv.RemoveFront(buf - recv.buf);
}
