#include "server.h"
#include "gpeer.h"
#include "vpeer.h"
#include "config.h"
#include "xx_logger.h"

#define S ((Server*)ec)

void GPeer::SendOpen(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "open", clientId);
}

void GPeer::SendKick(uint32_t const &clientId, int64_t const &delayMS) {
    LOG_INFO("clientId = ", clientId, " delayMS = ", delayMS);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "kick", clientId, delayMS);
}

void GPeer::SendClose(uint32_t const &clientId) {
    LOG_INFO("clientId = ", clientId);
    assert(clientIds.contains(clientId));
    SendTo(0xFFFFFFFFu, "close", clientId);
}

bool GPeer::Close(int const &reason, std::string_view const &desc) {
    // 防重入( 同时关闭 fd )
    if (!this->Item::Close(reason, desc)) return false;
    LOG_INFO("gatewayId = ", gatewayId, ", reason = ", reason, ", desc = ", desc);

    if (gatewayId != 0xFFFFFFFFu) {
        // close all child vpeer
        for (auto &&clientId: clientIds) {
            auto p = GetVPeerByClientId(clientId);
            assert(p);
            p->Kick(__LINE__, "GPeer Close", true);
        }

        // 从容器移除 this
        assert(S->gps.find(gatewayId) != S->gps.end());
        S->gps.erase(gatewayId);

        // set flag
        gatewayId = 0xFFFFFFFFu;
    }

    // 延迟减持
    DelayUnhold();
    return true;
}

void GPeer::Receive() {
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
            Close(__LINE__, "GPeer Receive if (dataLen < sizeof(addr) || dataLen > 1024 * 256)");
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
                // 普通包. serial + typeid + data...
                ReceivePackage(addr, buf + sizeof(addr), dataLen - sizeof(addr));
            }

            // 如果当前类实例 fd 已 close 则退出
            if (!Alive()) return;
        }
        // 跳到下一个包的开头
        buf += dataLen;
    }

    // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
    recv.RemoveFront(buf - recv.buf);
}

void GPeer::ReceivePackage(uint32_t const &clientId, uint8_t *const &buf, size_t const &len) {
    assert(gatewayId != 0xFFFFFFFFu);
    xx::Data_r dr(buf, len);
    LOG_INFO("gatewayId:", gatewayId, ", clientId = ", clientId, ", buf = ", dr);
    if (auto p = GetVPeerByClientId(clientId)) {
        p->Receive(dr.buf + dr.offset, dr.len - dr.offset);
    }
    // if not find, ignore
}

void GPeer::ReceiveCommand(uint8_t *const &buf, size_t const &len) {
    xx::Data_r dr(buf, len);
    std::string cmd;
    if (int r = dr.Read(cmd)) {
        LOG_ERR("gatewayId = ", gatewayId, " dr.Read(cmd) r = ", r);
        return;
    }
    if (gatewayId == 0xFFFFFFFFu) {
        if (cmd == "gatewayId") {
            if (int r = dr.Read(gatewayId)) {
                Close(__LINE__, xx::ToString("GPeer ReceiveCommand if (int r = dr.Read(gatewayId)), r = ", r));
                return;
            }
            auto iter = S->gps.find(gatewayId);
            if (iter != S->gps.end()) {
                Close(__LINE__, xx::ToString("GPeer ReceiveCommand cmd gatewayId = ", gatewayId, " already exists"));
                return;
            }
            SetTimeoutSeconds(15);
            S->gps[gatewayId] = xx::SharedFromThis(this);
            LOG_INFO("cmd = gatewayId = ", gatewayId);
            return;
        } else {
            LOG_ERR("unhandled cmd = ", cmd);
        }
    }
    else {
        if (cmd == "accept") {
            uint32_t clientId;
            if (int r = dr.Read(clientId)) {
                LOG_ERR("gatewayId = ", gatewayId, " accept dr.Read(clientId) r = ", r);
                return;
            }
            std::string ip;
            if (int r = dr.Read(ip)) {
                LOG_ERR("gatewayId = ", gatewayId, " accept dr.Read(ip) r = ", r);
                return;
            }
            // create peer, fill props, hold & store to server.vps, send open
            LOG_INFO("gatewayId = ", gatewayId, " accept clientId = ", clientId, " ip = ", ip);

            // todo: check ip is valid?

            (void) xx::Make<VPeer>(S, this, clientId, std::move(ip));
        } else if (cmd == "close") {
            uint32_t clientId;
            if (int r = dr.Read(clientId)) {
                LOG_ERR("gatewayId = ", gatewayId, " close dr.Read(clientId) r = ", r);
                return;
            }
            if (auto p = GetVPeerByClientId(clientId)) {
                p->Kick(__LINE__, xx::ToString("GPeer ReceiveCommand gatewayId = ", gatewayId, " close clientId = ", clientId));
            }
        } else if (cmd == "ping") {
            // keep alive
            SetTimeoutSeconds(15);
            // echo back
            SendTo(0xFFFFFFFFu, "ping", dr.LeftSpan());
        } else {
            LOG_ERR("gatewayId = ", gatewayId, " unhandled cmd = ", cmd);
        }
    }
}

VPeer *GPeer::GetVPeerByClientId(uint32_t const &clientId) const {
    assert(gatewayId != 0xFFFFFFFFu);
    assert(clientIds.contains(clientId));
    int idx = S->vps.Find<0>(((uint64_t) gatewayId << 32) | clientId);
    if (idx >= 0) return S->vps.ValueAt(idx).pointer;
    return nullptr;
}
