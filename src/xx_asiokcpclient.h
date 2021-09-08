#pragma once
#include <asio.hpp>
#include <ikcp.h>
#include <sstream>
#include <deque>
#include <iostream>
#include <xx_string.h>
#include <xx_ptr.h>
#include <xx_obj.h>

// simple asio udp + kcp client ( cpp only )

namespace xx {
    // 适配 asio::ip::address
    template<typename T>
    struct StringFuncs<T, std::enable_if_t<std::is_base_of_v<asio::ip::address, T>>> {
        static inline void Append(std::string &s, T const &in) {
            std::stringstream ss;
            ss << in;
            s.append(ss.str());
        }
    };

    struct AsioKcpPeer;

    // 带域名解析与拨号功能的 udp/kcp client
    struct AsioKcpClient {
        AsioKcpClient(AsioKcpClient const &) = delete;

        AsioKcpClient &operator=(AsioKcpClient const &) = delete;

        // configs
        std::string domain;
        int port;

        // ctxs
        asio::io_context ioc;
        asio::ip::udp::resolver resolver;
        std::vector<asio::ip::address> addrs;
        bool resolving = false;
        std::vector<Shared<AsioKcpPeer>> dials;
        Shared<AsioKcpPeer> peer;
        inline static uint32_t shakeSerial = 0;

        // 包管理器( 提供序列化功能 )
        ObjManager om;
        // for send
        Data d;
        // 所有已收到的包队列
        std::deque<xx::ObjBase_s> receivedPackages;

        AsioKcpClient() : ioc(1), resolver(ioc) {
            d.Reserve(8192);
        }

        // 每帧来一发, 以驱动底层收发，消息放入队列
        void Update();

        void SetDomainPort(std::string_view const &domain_, int const &port_) {
            assert(!Busy());
            domain = domain_;
            port = port_;
        }

        int Resolve() {
            resolver.cancel();
            addrs.clear();
            resolving = true;
            resolver.async_resolve(domain, "", [this](asio::error_code const &error, asio::ip::udp::resolver::results_type const &results) {
                resolving = false;
                if (!error.value()) {
                    for (auto &&r: results) {
                        addrs.emplace_back(r.endpoint().address());
                    }
                }
            });
            return 0;
        }

        void Reset() {
            addrs.clear();
            resolver.cancel();
            resolving = false;
            dials.clear();
            peer.Reset();
            receivedPackages.clear();
        }

        [[nodiscard]] bool Busy() const {
            return resolving || !dials.empty();
        }

        [[nodiscard]] bool Alive() const {
            return (bool) peer;
        }

        [[nodiscard]] bool IsResolved() const {
            return !addrs.empty();
        }

        [[nodiscard]] std::vector<std::string> GetIPList() const {
            std::vector<std::string> r;
            for (auto &a: addrs) {
                xx::Append(r.emplace_back(), a);
            }
            return r;
        }

        int Dial() {
            if (Busy() || Alive()) return __LINE__;
            dials.clear();
            for (auto &&a: addrs) {
                dials.emplace_back(Make<AsioKcpPeer>(this, asio::ip::udp::endpoint(a, port), ++shakeSerial));
            }
            return 0;
        }

        // 发包
        int Send(xx::ObjBase_s const &o);

        // 尝试 move 出一条最前面的消息
        [[nodiscard]] bool TryGetPackage(xx::ObjBase_s &pkg);
    };

    struct AsioKcpPeer {
        AsioKcpClient *client;
        asio::ip::udp::socket socket;
        asio::ip::udp::endpoint ep;
        ikcpcb *kcp = nullptr;
        uint32_t conv = 0;
        uint32_t shakeSerial = 0;
        int64_t kcpCreateMS = 0;    // 兼做 shake 握手延迟计数

        // for recv from kcp
        Data recv;
        // for recv from udp
        char udpRecvBuf[1024 * 64];


        // 初始化非阻塞 udp sokcet
        explicit AsioKcpPeer(AsioKcpClient *const &c, asio::ip::udp::endpoint const &ep, uint32_t const &shakeSerial)
                : client(c), socket(c->ioc, asio::ip::udp::endpoint(ep.protocol(), 0)), ep(ep), shakeSerial(shakeSerial) {
            socket.non_blocking(true);
            recv.Reserve(1024 * 256);
        }

        // 回收 kcp
        ~AsioKcpPeer() {
            if (kcp) {
                ikcp_release(kcp);
                kcp = nullptr;
            }
        }

        int UdpSend(void const *const &buf, size_t const &len) {
            //xx::CoutN("udp send = ", xx::Span(buf, len));
            try {
                auto sentLen = socket.send_to(asio::buffer(buf, len), ep);
                return sentLen == len ? 0 : __LINE__;
            }
            catch (asio::system_error const &/* e */) {
                // todo: log?
                return __LINE__;
            }
        }

        // 返回 0 表示正常, 返回 1 表示连接成功( 拨号状态 ). 别的值为错误码
        int Update(int64_t const &ms) {
            if (!kcp) {
                // 时间到就发送握手包
                if (ms > kcpCreateMS) {
                    if (int r = UdpSend(&shakeSerial, sizeof(shakeSerial))) return r;
                    // 下次发送在 0.2 秒后
                    kcpCreateMS = ms + 200;
                }
            } else {
                ikcp_update(kcp, (IUINT32) (ms - kcpCreateMS));
            }

            asio::error_code e;
            asio::ip::udp::endpoint p;
            // 如果有 udp 数据收到
            if (auto recvLen = socket.receive_from(asio::buffer(udpRecvBuf), p, 0, e)) {
                // xx::CoutN("udp recvLen = ", recvLen);
                if (kcp) {
                    // 准备向 kcp 灌数据并收包放入 recvs
                    // 前置检查. 如果数据长度不足( kcp header ), 或 conv 对不上就 忽略
                    if (recvLen < 24 || conv != *(uint32_t *) (udpRecvBuf)) return 0;

                    // 将数据灌入 kcp. 灌入出错则 Close
                    if (int r = ikcp_input(kcp, udpRecvBuf, (long) recvLen)) return __LINE__;

                    // 开始处理收到的数据
                    do {
                        // 如果数据长度 == buf限长 就自杀( 未处理数据累计太多? )
                        if (recv.len == recv.cap) return __LINE__;

                        // 从 kcp 提取数据. 追加填充到 recv 后面区域. 返回填充长度. <= 0 则下次再说
                        auto &&len = ikcp_recv(kcp, (char *) recv.buf + recv.len, (int) (recv.cap - recv.len));
                        if (len <= 0) return 0;
                        recv.len += len;

                        // 开始切包放 recvs
                        // 取出指针备用
                        auto buf = (uint8_t *) recv.buf;
                        auto end = (uint8_t *) recv.buf + recv.len;
                        //xx::CoutN("recv = ", recv);

                        // 包头
                        uint32_t dataLen = 0;

                        // 确保包头长度充足
                        while (buf + sizeof(dataLen) <= end) {
                            // 读包头 / 数据长
                            dataLen = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

                            // 数据未接收完 就 跳出
                            if (buf + sizeof(dataLen) + dataLen > end) break;

                            // 跳到数据区开始调用处理回调
                            buf += sizeof(dataLen);
                            {
                                // 解包 & 存队列
                                xx::Data_r dr(buf, dataLen);
                                xx::ObjBase_s ob;
                                if (int r = client->om.ReadFrom(dr, ob)) {
                                    client->om.KillRecursive(ob);
                                } else {
                                    //client->om.CoutN("recv = ", ob);
                                    client->receivedPackages.push_back(std::move(ob));
                                }
                            }

                            // 跳到下一个包的开头
                            buf += dataLen;
                        }

                        // 移除掉已处理的数据( 将后面剩下的数据移动到头部 )
                        recv.RemoveFront(buf - (uint8_t *) recv.buf);

                    } while (true);
                } else {
                    // 判断握手信息正确性, 切割出 conv
                    if (recvLen == 8 && *(uint32_t *) udpRecvBuf == shakeSerial) {
                        conv = *(uint32_t *) (udpRecvBuf + 4);

                        // 记录创建毫秒数 for ikcp_update
                        kcpCreateMS = ms;

                        // 创建并设置 kcp 的一些参数
                        kcp = ikcp_create(conv, this);
                        (void) ikcp_wndsize(kcp, 1024, 1024);
                        (void) ikcp_nodelay(kcp, 1, 10, 2, 1);
                        kcp->rx_minrto = 10;
                        kcp->stream = 1;

                        // 给 kcp 绑定 output 功能函数
                        ikcp_setoutput(kcp, [](const char *buf, int len, ikcpcb *_, void *user) -> int {
                            return ((AsioKcpPeer *) user)->UdpSend(buf, len);
                        });

                        // 先 update 来一发 确保 Flush 生效( 和 kcp 内部实现的一个标志位有关 )
                        ikcp_update(kcp, 0);

                        // 通过 kcp 发 accept 触发包
                        Send((uint8_t*)"\1\0\0\0\0", 5);

                        // 返回拨号成功( 接下来需要立刻给 server 发点啥以触发逻辑 accept )
                        return 1;
                    }
                }
            }
            return 0;
        }

        // 基础发送函数
        int Send(uint8_t const *const &buf, size_t const &len) {
            //xx::CoutN("ikcp_send data = ", xx::Span(buf, len));
            if (int r = ikcp_send(kcp, (char *) buf, (int) len)) return r;
            ikcp_flush(kcp);
            return 0;
        }

        // 立刻发出
        int Flush() {
            if (!kcp) return __LINE__;
            ikcp_flush(kcp);
            return 0;
        }
    };

    [[nodiscard]] inline bool AsioKcpClient::TryGetPackage(xx::ObjBase_s &pkg) {
        if (!Alive()) return false;
        if (receivedPackages.empty()) return false;
        pkg = std::move(receivedPackages.front());
        receivedPackages.pop_front();
        return true;
    }

    inline void AsioKcpClient::Update() {
        if (ioc.stopped()) {
            ioc.restart();
        }
        ioc.poll();

        auto ms = NowSteadyEpochMilliseconds();

        // 已连接( 和拨号互斥 )
        if (peer) {
            // 更新。连接出错则 Reset
            if (int r = peer->Update(ms)) {
                // todo: log?
                Reset();
            }
        }
            // 拨号
        else if (!dials.empty()) {
            // 倒序 for 方便交换删除
            for (auto i = (int) dials.size() - 1; i >= 0; --i) {
                assert(dials[i]);
                if (int r = dials[i]->Update(ms)) {
                    if (r == 1) {
                        // 拨号成功：挪到 peer 并清除别的拨号, 退出 for
                        peer = std::move(dials[i]);
                        dials.clear();
                        break;
                    } else {
                        // 逻辑交换删除
                        dials[i] = std::move(dials.back());
                        dials.pop_back();
                    }
                }
            }
        }
    }

    inline int AsioKcpClient::Send(xx::ObjBase_s const &o) {
        if (!Alive()) return -1;
        d.Clear();
        auto bak = d.WriteJump<false>(sizeof(uint32_t));
        om.WriteTo(d, o);
        d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
        if (int r = peer->Send(d.buf, d.len)) return r;
        return peer->Flush();
    }
}
