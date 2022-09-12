#pragma once

#include "xx_obj.h"
#include "ikcp.h"

#ifdef USE_STANDALONE_ASIO
#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
namespace boostsystem = asio;
#else
#include <boost/asio.hpp>
#include <boost/asio/experimental/as_tuple.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
namespace asio = boost::asio;
namespace boostsystem = boost::system;
#endif

using namespace asio::experimental::awaitable_operators;
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);
using asio::co_spawn;
using asio::awaitable;
using asio::detached;



// 为 cocos / unity 之类客户端 实现一个带 域名解析, kcp拨号 与 通信 的 网关拨号版本。功能函数和用法，与 xx::UvClient 封装的 lua 接口类似

// Lua:

// void Update())                                               -- 独立执行, 每帧开头来一发

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

// void SetCppServiceId(int serviceId)                          -- 为 C++ 代码处理的服务设置id
// [serviceId, serial, data] TryGetPackage()                    -- Lua 拉取数据包

// C++:

// HandleReceivedCppPackages 为主线驱动. 走事件模式

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

        void SetDomainPort(std::string_view const &domain_, int const &port_) {
            assert(!Busy());
            domain = domain_;
            port = port_;
        }

        int Resolve() {
            resolver.cancel();
            addrs.clear();
            resolving = true;
            resolver.async_resolve(domain, "", [this](boostsystem::error_code const &error, asio::ip::udp::resolver::results_type const &results) {
                resolving = false;
                if (!error.value()) {
                    for (auto &&r : results) {
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
        }

        [[nodiscard]] bool Busy() const {
            return resolving || !dials.empty();
        }

        [[nodiscard]] bool Alive() const {
            return (bool)peer;
        }

        [[nodiscard]] bool IsResolved() const {
            return !addrs.empty();
        }

        [[nodiscard]] std::vector<std::string> GetIPList() const {
            std::vector<std::string> r;
            for (auto &a : addrs) {
                xx::Append(r.emplace_back(), a);
            }
            return r;
        }

        int Dial() {
            if (Busy() || Alive()) return __LINE__;
            dials.clear();
            for (auto &&a : addrs) {
                dials.emplace_back(Make<KcpPeer>(this, asio::ip::udp::endpoint(a, port), ++shakeSerial));
            }
            return 0;
        }

        // 尝试 move 出一条最前面的消息( lua 那边则将 Package 变为 table, 以成员方式压栈. 之后分发 )
        [[nodiscard]] bool TryGetPackage(Package& pkg);

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

        int UdpSend(void const* const& buf, size_t const& len) {
            //xx::CoutN("udp send = ", xx::Span(buf, len));
            try {
                auto sentLen = socket.send_to(asio::buffer(buf, len), ep);
                return sentLen == len ? 0 : __LINE__;
            }
            catch (boostsystem::system_error const &/*e*/) {
                // todo: log?
                return __LINE__;
            }
        }

        // 返回 0 表示正常, 返回 1 表示连接成功( 拨号状态 ). 别的值为错误码
        int Update(int64_t const& ms) {
            if (!kcp) {
                // 时间到就发送握手包
                if (ms > kcpCreateMS) {
                    if (int r = UdpSend(&shakeSerial, sizeof(shakeSerial))) return r;
                    // 下次发送在 0.2 秒后
                    kcpCreateMS = ms + 200;
                }
            }
            else {
                ikcp_update(kcp, (IUINT32) (ms - kcpCreateMS));
            }

            boostsystem::error_code e;
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
                        //xx::CoutN("recv = ", recv);

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
                                auto bb = xx::Data_r(buf + sizeof(dataLen), dataLen);
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
                                        data.WriteBuf(bb.buf + bb.offset, bb.len - bb.offset);
                                        receivedPackages.emplace_back(serviceId, 0, std::move(data));
                                        //xx::CoutN("recv echo");
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
                                    data.WriteBuf(bb.buf + bb.offset, bb.len - bb.offset);
                                    receivedPackages.emplace_back(serviceId, serial, std::move(data));
                                }
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
                            return ((KcpPeer *) user)->UdpSend(buf, len);
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
        int Send(uint8_t const *const &buf, size_t const &len) {
            //xx::CoutN("Send data = ", xx::Span(buf, len));
            if (int r = ikcp_send(kcp, (char*)buf, (int)len)) return r;
            ikcp_flush(kcp);
            return 0;
        }

        // 向某 service 发包( 结构：4字节len, 4字节serviceId, 变长serial, 不带长度data内容 )
        // 如果 serviceId == 0xFFFFFFFFu 则向网关发 echo 指令 ( 不写变长serial, 写 "echo" )
        int SendTo(uint32_t const& serviceId, int32_t const& serial, Data const& data) {
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
            tmp.WriteFixedAt(bak, (uint32_t)(tmp.len - 4));
            //xx::CoutN("tmp = ", tmp);
            return Send(tmp.buf, tmp.len);
        }

        // 向网关发 echo 指令
        int SendEcho(Data const& data) {
            return SendTo(0xFFFFFFFFu, 0, data);
        }

        /********************************************************************************************/
        // 下面代码 for CPP 处理消息

        using CB = std::function<void(int const& serial_, ObjBase_s&& ob)>;

        // C++ 服务id( 同时也是 C++ 调用 SendXxxxxx 时的 默认 serviceId
        uint32_t cppServiceId = 0xFFFFFFFFu;
        // C++ 代码处理的包之专用容器。 TryGetPackage 过程中检测到 serviceId 等于 cppServiceId 中则从 receivedPackages 挪过来
        std::deque<Package> receivedCppPackages;
        // for cpp s/de erial
        ObjManager om;
        // for RPC
        int rpcSerial = 0;
        // for SendRequest. int: serial   double: timeoutSeconds
        std::vector<std::tuple<int, double, CB>> callbacks;
        // events
        std::function<void(ObjBase_s&& ob)> onReceivePush = [](auto&& ob){};
        CB onReceiveRequest = [](int const& serial, ObjBase_s&& msg){};
        std::function<void(Data const& d)> onReceiveEcho = [](Data const& d){};

        // 发回应
        template<typename PKG = xx::ObjBase, typename ... Args>
        void SendResponse(int32_t const& serial, Args const &... args) {
            tmp.Clear();
            tmp.Reserve(8192);
            auto bak = tmp.WriteJump<false>(sizeof(uint32_t));
            tmp.WriteFixed<false>(cppServiceId);
            tmp.WriteVarInteger<false>(serial);
            // 传统写包
            if constexpr (std::is_same_v<xx::ObjBase, PKG>) {
                om.WriteTo(tmp, args...);
            }
            // 直写 cache buf 包
            else if constexpr (std::is_same_v<xx::Span, PKG>) {
                tmp.WriteBufSpans(args...);
            }
            // 使用 目标类的静态函数 快速填充 buf
            else {
                PKG::WriteTo(tmp, args...);
            }
            tmp.WriteFixedAt(bak, (uint32_t)(tmp.len - 4));
            Send(tmp.buf, tmp.len);
        }

        // 发推送
        template<typename PKG = xx::ObjBase, typename ... Args>
        void SendPush(Args const& ... args) {
            // 推送性质的包, serial == 0
            this->template SendResponse<PKG>(0, args...);
        }

        // 发请求（收到相应回应时会触发 cb 执行。超时或断开也会触发，o == nullptr）
        template<typename PKG = xx::ObjBase, typename ... Args>
        int SendRequest(CB&& cb, double const& timeoutSeconds, Args const& ... args) {
            rpcSerial = (rpcSerial + 1) & 0x7FFFFFFF;
            SendResponse<PKG>(-rpcSerial, args...);
            callbacks.emplace_back(rpcSerial, NowSteadyEpochSeconds() + timeoutSeconds, std::move(cb));
            return rpcSerial;
        }


        // 设置 cpp 这边要处理的服务的 id 以便截获 pkg
        void SetCppServiceId(uint32_t const& cppServiceId_) {
            assert(cppServiceId_ != 0xFFFFFFFFu);
            this->cppServiceId = cppServiceId_;
        }

        // 处理 cpp pkg
        virtual void HandleReceivedCppPackages() noexcept {
            // dead checker
            auto holder = &client->peer;

            // 处理所有消息
            while (!receivedCppPackages.empty()) {
                auto& p = receivedCppPackages.front();

                if (p.serviceId == 0xFFFFFFFFu) {
                    assert(p.serial == 0);
                    onReceiveEcho(p.data);
                }
                else {
                    ObjBase_s msg;
                    if (!om.ReadFrom(p.data, msg)) {
                        if (p.serial == 0) {
                            onReceivePush(std::move(msg));
                        }
                        else if (p.serial < 0) {
                            onReceiveRequest(-p.serial, std::move(msg));
                        }
                        else {
                            for (int i = (int)callbacks.size() - 1; i >= 0; --i) {
                                auto& c = callbacks[i];
                                // 找到：交换删除并执行( 参数传msg ), break
                                if (std::get<0>(c) == p.serial) {
                                    auto a = std::move(std::get<2>(c));
                                    c = std::move(callbacks.back());
                                    callbacks.pop_back();
                                    a(p.serial, std::move(msg));
                                    break;
                                }
                            }
                            // 如果没找到，可能已超时，不需要处理
                        }
                    }
                }

                // check if Reset
                if (!*holder) return;

                receivedCppPackages.pop_front();
            }

            // 处理回调超时
            if (!callbacks.empty()) {
                auto nowSecs = NowSteadyEpochSeconds();
                for (int i = (int)callbacks.size() - 1; i >= 0; --i) {
                    auto& c = callbacks[i];
                    // 超时：交换删除并执行( 空指针参数 )
                    if (std::get<1>(c) < nowSecs) {
                        auto serial = std::get<0>(c);
                        auto a = std::move(std::get<2>(c));
                        c = std::move(callbacks.back());
                        callbacks.pop_back();
                        a(serial, nullptr);
                    }

                    // check if Reset
                    if (!*holder) return;
                }
            }
        }
    };

    [[nodiscard]] inline bool Client::TryGetPackage(Package& pkg) {
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
                    }
                    else {
                        // 逻辑交换删除
                        dials[i] = std::move(dials.back());
                        dials.pop_back();
                    }
                }
            }
        }
    }
}



//            socket.send_to(asio::buffer("asdf", 4), ep);
//            char udpRecvBuf[1024 * 64];
//            asio::error_code e;
//            asio::ip::udp::endpoint p;
//            auto recvLen = socket.receive_from(asio::buffer(udpRecvBuf), p, 0, e);
//            xx::CoutN(recvLen);

//            auto lp = socket.local_endpoint();
//            assert(ep.protocol() == lp.protocol());
//            xx::CoutN(lp.address(), ":", lp.port(), " to ", ep.address(), ":", ep.port());
