#pragma once

#include "xx_obj.h"
#include <asio.hpp>
#include "ikcp.h"
#include <sstream>
#include <deque>

// 为 cocos / unity 之类客户端 实现一个带 域名解析, kcp拨号 与 通信 的 网关拨号版本。功能函数和用法，与 xx::UvClient 封装的 lua 接口类似

// void Update())                                               -- 每帧来一发

// void SetDomainPort(string domainName, int port)              -- 设置 域名/ip 和 端口

// int Resolve()                                                -- 开始域名解析
// bool IsResolved()                                            -- 判断解析后的域名列表是否为空( 空则解析失败 )
// string[] GetIPList()                                         -- 获取解析后的域名列表 for dump

// void Reset())                                                -- 停止解析 或 拨号 或 连接设空, 清除所有数据

// int Dial()                                                   -- 拨号
// bool Busy())                                                 -- 是否正在解析或拨号
// bool Alive()                                                 -- 连接是否存在( 因为是 udp, 故不知道是否断开, 得靠自己超时检测 & 主动 Disconnect )
// bool IsOpened(int serviceId)                                 -- 服务开启检查

// int SendTo(int serviceId, int serial, xx::Data data)         -- 主要的数据发送指令
// int SendEcho(xx::Data data);                                 -- 向网关直发 echo 数据（ 通常拿来做 ping )

// void SetGroup(int serviceId, int groupId)                    -- 为服务id分组. 不设置默认为 0
// [serviceId, serial, data] TryGetPackage(int groupId = 0)     -- 获取分组数据包

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
}

namespace xx::Asio {

    struct KcpPeer;

    // 包容器( 不封装到 lua )
    struct Package {
        Package() = default;
        Package(Package const&) = default;
        Package& operator=(Package const&) = default;
        Package(Package&& o) = default;
        Package& operator=(Package&& o) = default;

        uint32_t serviceId = 0;
        int serial = 0;
        Data data;

        Package(uint32_t const& serviceId, int const& serial, Data&& data)
                : serviceId(serviceId), serial(serial), data(std::move(data)) {}
    };

    // 全功能客户端( 封装到 lua )
    struct Client {
        Client(Client const &) = delete;

        Client &operator=(Client const &) = delete;

        // configs
        std::string domain;
        int port;

        // ctxs
        asio::io_context ioc;
        asio::ip::udp::resolver resolver;
        std::vector<asio::ip::address> addrs;
        bool resolving = false;
        std::vector<Shared<KcpPeer>> dials;
        Shared<KcpPeer> peer;
        inline static uint32_t shakeSerial = 0;

        Client() : ioc(1), resolver(ioc) {}

        // 每帧来一发, 以驱动底层收发，消息放入综合队列 ( for lua / cpp )
        void Update();

        inline void SetDomainPort(std::string_view const &domain_, int const &port_) {
            assert(!Busy());
            domain = domain_;
            port = port_;
        }

        inline int Resolve() {
            resolver.cancel();
            addrs.clear();
            resolving = true;
            resolver.async_resolve(domain, "", [this](asio::error_code const &error, asio::ip::udp::resolver::results_type const &results) {
                resolving = false;
                if (!error.value()) {
                    for (auto &&r : results) {
                        addrs.emplace_back(r.endpoint().address());
                    }
                }
            });
            return 0;
        }

        inline void Reset() {
            addrs.clear();
            resolver.cancel();
            resolving = false;
            dials.clear();
            peer.Reset();
        }

        [[nodiscard]] inline bool Busy() const {
            return resolving || !dials.empty();
        }

        [[nodiscard]] inline bool Alive() const {
            return (bool)peer;
        }

        [[nodiscard]] inline bool IsResolved() const {
            return !addrs.empty();
        }

        [[nodiscard]] inline std::vector<std::string> GetIPList() const {
            std::vector<std::string> r;
            for (auto &a : addrs) {
                xx::Append(r.emplace_back(), a);
            }
            return r;
        }

        inline void Dial() {
            assert(!Busy() && !Alive());
            for (auto &&a : addrs) {
                dials.emplace_back(Make<KcpPeer>(this, asio::ip::udp::endpoint(a, port), ++shakeSerial));
            }
        }

        // 尝试 move 出一条最前面的消息( lua 那边则将 Package 变为 table, 以成员方式压栈. 之后分发 )
        [[nodiscard]] bool TryGetPackage(Package& pkg, int const& groupId = 0);

        [[nodiscard]] bool IsOpened(uint32_t const& serviceId) const;
    };

    struct KcpPeer {
        Client *client;
        asio::ip::udp::socket socket;
        asio::ip::udp::endpoint ep;
        ikcpcb *kcp = nullptr;
        uint32_t conv = 0;
        uint32_t shakeSerial = 0;
        int64_t kcpCreateMS = 0;    // 兼做 shake 握手延迟计数

        // for send
        Data tmp;
        // for recv from kcp
        Data recv;
        // for recv from udp
        char udpRecvBuf[1024 * 64];

        // 已 open 服务id表
        std::unordered_set<uint32_t> openServerIds;
        // 所有已收到的数据队列
        std::deque<Package> receivedPackages;



        // 初始化 sokcet
        explicit KcpPeer(Client *const &c, asio::ip::udp::endpoint const &ep, uint32_t const &shakeSerial)
                : client(c), socket(c->ioc, asio::ip::udp::endpoint(ep.protocol(), 0)), ep(ep), shakeSerial(shakeSerial) {
            socket.non_blocking(true);
            recv.Reserve(1024 * 256);
        }

        // 回收 kcp
        ~KcpPeer() {
            if (kcp) {
                ikcp_release(kcp);
                kcp = nullptr;
            }
        }

        // 返回 0 表示正常, 返回 1 表示连接成功( 拨号状态 ). 别的值为错误码
        inline int Update() {
            auto ms = NowSteadyEpochMilliseconds();
            if (!kcp) {
                // 时间到就发送握手包
                if (ms > kcpCreateMS) {
                    try {
                        socket.send_to(asio::buffer(&shakeSerial, sizeof(shakeSerial)), ep);
                        // 下次发送在 0.2 秒后
                        kcpCreateMS = ms + 200;
                    }
                    catch (asio::system_error const &e) {
                        // todo: log?
                        return __LINE__;
                    }
                }
            }
            else {
                ikcp_update(kcp, (IUINT32) (ms - kcpCreateMS));
            }
            asio::error_code e;
            asio::ip::udp::endpoint p;
            // 如果有 udp 数据收到
            if (auto recvLen = socket.receive_from(asio::buffer(udpRecvBuf), p, 0, e)) {
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

                        // 包头
                        uint32_t dataLen = 0;

                        // 确保包头长度充足
                        while (buf + sizeof(dataLen) <= end) {
                            // 读包头 / 数据长
                            dataLen = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

                            // 计算包总长( 包头长 + 数据长 )
                            auto totalLen = sizeof(dataLen) + dataLen;

                            // 如果包不完整 就 跳出
                            if (buf + totalLen > end) break;

                            {
                                auto bb = xx::Data_r(buf, len);
                                // 最上层结构：4字节长度 + 4字节服务id + 其他
                                uint32_t serviceId = 0;
                                if (bb.ReadFixed(serviceId)) return __LINE__;
                                // 服务id 如果为 0xFFFFFFFFu 则为内部指令：string cmd + args
                                if (serviceId == 0xFFFFFFFFu) {
                                    std::string_view cmd;
                                    if (bb.Read(cmd)) return __LINE__;
                                    if (cmd == "open") {
                                        if (bb.Read(serviceId)) return __LINE__;
                                        openServerIds.insert(serviceId);
                                    }
                                    else if (cmd == "close") {
                                        if (bb.Read(serviceId)) return __LINE__;
                                        openServerIds.erase(serviceId);
                                    }
                                    else if (cmd == "echo") {
                                        Data data;
                                        data.WriteBuf(buf + bb.offset, len - bb.offset);
                                        receivedPackages.emplace_back(serviceId, 0, std::move(data));
                                    }
                                    else {
                                        return __LINE__;
                                    }
                                }
                                else {
                                    // 其他结构：int serial + PKG serial data
                                    int serial = 0;
                                    if(bb.Read(serial)) return __LINE__;
                                    // 剩余数据打包为 Data 塞到收包队列
                                    Data data;
                                    data.WriteBuf(buf + bb.offset, len - bb.offset);
                                    receivedPackages.emplace_back(serviceId, serial, std::move(data));
                                }
                                return 0;
                            }

                            // 跳到下一个包的开头
                            buf += totalLen;
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
                            return (int) ((KcpPeer *) user)->socket.send_to(asio::buffer(buf, len), ((KcpPeer *) user)->ep);
                        });

                        // 先 update 来一发 确保 Flush 生效( 和 kcp 内部实现的一个标志位有关 )
                        ikcp_update(kcp, 0);

                        // 通过 kcp 发 accept 触发包
                        Send((uint8_t*)"\1\0\0\0\0", 5);

                        // 返回拨号成功
                        return 1;
                    }
                }
            }
            return 0;
        }

        // 基础函数
        inline int Send(uint8_t const *const &buf, size_t const &len) {
            if (int r = ikcp_send(kcp, (char*)buf, (int)len)) return r;
            ikcp_flush(kcp);
            return 0;
        }

        // 向某 service 发包( 结构：4字节len, 4字节serviceId, 变长serial, 不带长度data内容 )
        // 如果 serviceId == 0xFFFFFFFFu 则向网关发 echo 指令 ( 不写变长serial, 写 "echo" )
        inline int SendTo(uint32_t const& serviceId, int32_t const& serial, Data const& data) {
            tmp.Clear();
            tmp.Reserve(13 + data.len);
            auto bak = tmp.WriteJump<false>(sizeof(uint32_t));
            tmp.WriteFixed<false>(serviceId);
            if (serviceId != 0xFFFFFFFFu) {
                tmp.WriteVarInteger<false>(serial);
            }
            else {
                tmp.Write<false>("echo");
            }
            tmp.WriteBuf<false>(data.buf, data.len);
            tmp.WriteFixedAt(bak, (uint32_t)tmp.len);
            return Send(tmp.buf, tmp.len);
        }

        /********************************************************************************************/
        // 下面代码 for CPP 处理消息

        // C++ 服务id( 同时也是 C++ 调用 SendXxxxxx 时的 默认 serviceId
        uint32_t cppServiceId = 0xFFFFFFFFu;
        // C++ 代码处理的包之专用容器。 TryGetPackage 过程中检测到 serviceId 等于 cppServiceId 中则从 receivedPackages 挪过来
        std::deque<Package> receivedCppPackages;
        // for cpp s/de erial
        ObjManager om;
        // for RPC
        int rpcSerial = 0;
        // for SendRequest. int: serial   int64_t: timeoutMS
        std::vector<std::tuple<int, int64_t, std::function<int(ObjBase_s&& msg)>>> callbacks;
        // events
        std::function<int(ObjBase_s&& msg)> onReceivePush = [](auto&& msg){ return 0; };
        std::function<int(int const& serial, ObjBase_s&& msg)> onReceiveRequest = [](int const& serial, ObjBase_s&& msg){ return 0; };


        inline void SetCppServiceId(uint32_t const& cppServiceId_) {
            assert(cppServiceId_ != 0xFFFFFFFFu);
            this->cppServiceId = cppServiceId_;
        }

        inline int SendPush(ObjBase_s const& msg) {
            return SendResponse(0, msg);
        }

        inline int SendResponse(int32_t const& serial, ObjBase_s const& msg) {
            tmp.Clear();
            tmp.Reserve(8192);
            auto bak = tmp.WriteJump<false>(sizeof(uint32_t));
            tmp.WriteFixed<false>(cppServiceId);
            tmp.WriteVarInteger<false>(serial);
            om.WriteTo(tmp, msg);
            tmp.WriteFixedAt(bak, (uint32_t)tmp.len);
            return Send(tmp.buf, tmp.len);
        }

        inline int SendRequest(ObjBase_s const& msg, std::function<int(ObjBase_s&& msg)>&& cb, uint64_t const& timeoutMS) {
            rpcSerial = (rpcSerial + 1) & 0x7FFFFFFF;
            if (int r = SendResponse(-rpcSerial, msg)) return r;
            callbacks.emplace_back(rpcSerial, NowSteadyEpochMilliseconds() + (int64_t)timeoutMS, std::move(cb));
            return 0;
        }

        inline virtual int HandleReceivedCppPackages() noexcept {
            // 处理所有消息
            while (!receivedCppPackages.empty()) {
                auto& p = receivedCppPackages.front();

                ObjBase_s msg;
                if (int r = om.ReadFrom(p.data, msg)) return r;

                if (p.serial == 0) {
                    if (int r = onReceivePush(std::move(msg))) return r;
                }
                else if (p.serial < 0) {
                    if (int r = onReceiveRequest(-p.serial, std::move(msg))) return r;
                }
                else {
                    for (int i = (int)callbacks.size() - 1; i >= 0; --i) {
                        // 找到：交换删除并执行( 参数传msg ), break
                        if (std::get<0>(callbacks[i]) == p.serial) {
                            auto a = std::move(std::get<2>(callbacks[i]));
                            callbacks[i] = std::move(callbacks.back());
                            callbacks.pop_back();
                            if (int r = a(std::move(msg))) return r;
                            break;
                        }
                    }
                    // 如果没找到，可能已超时，不需要处理
                }

                receivedCppPackages.pop_front();
            }

            // 处理回调超时
            if (!callbacks.empty()) {
                auto nowMS = NowSteadyEpochMilliseconds();
                for (int i = (int)callbacks.size() - 1; i >= 0; --i) {
                    // 超时：交换删除并执行( 空指针参数 )
                    if (std::get<1>(callbacks[i]) < nowMS) {
                        auto a = std::move(std::get<2>(callbacks[i]));
                        callbacks[i] = std::move(callbacks.back());
                        callbacks.pop_back();
                        a(nullptr);
                    }
                }
            }
            return 0;
        }
    };

    [[nodiscard]] inline bool Client::TryGetPackage(Package& pkg, int const& groupId) {
        if (!Alive()) return false;
        auto&& ps = peer->receivedPackages;
        LabLoop:
        if (ps.empty()) return false;
        // 遇到需要在 cpp 中处理的包就移走继续判断下一个
        if ( peer->cppServiceId == ps.front().serviceId) {
            peer->receivedCppPackages.emplace_back(std::move(ps.front()));
            ps.pop_front();
            goto LabLoop;
        }
        pkg = std::move(ps.front());
        ps.pop_front();
        return true;
    }

    [[nodiscard]] inline bool Client::IsOpened(uint32_t const& serviceId) const {
        if (!peer) return false;
        return peer->openServerIds.contains(serviceId);
    }

    inline void Client::Update() {
        if (ioc.stopped()) {
            ioc.restart();
        }
        ioc.poll();

        // 已连接( 和拨号互斥 )
        if (peer) {
            // 更新。连接出错则 Reset
            if (int r = peer->Update()) {
                // todo: log?
                Reset();
            }
        }
        // 拨号
        else if (!dials.empty()) {
            // 倒序 for 方便交换删除
            for (auto i = (int) dials.size() - 1; i >= 0; --i) {
                assert(dials[i]);
                if (int r = dials[i]->Update()) {
                    if (r == 1) {
                        // 拨号成功：挪到 peer 并清除别的拨号, 退出 for
                        peer = std::move(dials[i]);
                        dials.clear();
                        break;
                    }
                    else {
                        // 逻辑交换删除
                        dials[i] = dials[dials.size()-1];
                        dials.pop_back();
                    }
                }
            }
        }
    }
}
