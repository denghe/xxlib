#pragma once

#include "xx_epoll.h"
#include "ikcp.h"

//#include <string_view>
//// 实现针对 sockaddr_in6 的 hash & operator== 以便于放入字典 免去 tostring 以及更高效算 hash?
//namespace std {
//    template<>
//    struct hash<sockaddr_in6> {
//        size_t operator()(sockaddr_in6 const& a) const {
//            // todo: 应该根据 addr 的 type 路由并具体到某一段数据针对性算 hash
//            return hash<std::string_view>()(std::string_view((char*)&a, sizeof(a)));
//        }
//    };
//}
//inline bool operator==(sockaddr_in6 const& a, sockaddr_in6 const& b) {
//    // todo: 应该根据 addr 的 type 路由并具体到某一段数据针对性 memcmp
//    return !memcmp(&a, &b, sizeof(a));
//}

namespace xx::Epoll {
    /***********************************************************************************************************/
    // 基础 udp 组件
    struct UdpPeer : Timer {
        // 对方的 addr( udp 收到数据时会填充. SendTo 时用到 )
        sockaddr_in6 addr{};

        // 配置每次 epoll_wait 读取事件通知后一口气最多读多少次. 如果是 Listener 性质, 该值还要改大很多
        int readCountAtOnce = 32;

        // 继承构造函数
        using Timer::Timer;

        // 有数据需要接收( 不累积, 直接投递过来 )
        virtual void Receive(uint8_t const *const &buf, size_t const &len) = 0;

        // 直接发送( 如果 fd == -1 则忽略 ), 返回已发送字节数. -1 为出错
        ssize_t Send(uint8_t const *const &buf, size_t const &len);

        // 直接向 addr 发送( 如果 fd == -1 则忽略 ), 返回已发送字节数. -1 为出错
        ssize_t SendTo(sockaddr_in6 const& addr, uint8_t const *const &buf, size_t const &len);

        // 判断 fd 的有效性
        inline bool Alive() { return fd != -1; }

        // 工具函数: 用指定参数创建 fd. 成功返回 0
        int MakeFD(int const &port = 0, char const* const& hostName = nullptr, bool const& reusePort = false, size_t const& rmem_max = 0, size_t const& wmem_max = 0);

        // 方便直接用这个类
        inline void Timeout() override {}

    protected:
        friend Context;

        // epoll 事件处理
        void EpollEvent(uint32_t const &e) override;
    };

    struct KcpPeer;

    struct KcpBase : UdpPeer {
        // 自增生成连接id
        uint32_t convId = 0;
        // kcp conv 值与 peer 的映射。KcpPeer Close 时从该字典移除 key
        tsl::hopscotch_map<uint32_t, KcpPeer *> cps;
        // 带超时的握手信息字典 key: ip:port   value: conv, nowMS
        tsl::hopscotch_map<std::string, std::pair<uint32_t, int64_t>> shakes;

        // 构造函数( 启动 timer 用作每帧驱动 update )
        explicit KcpBase(Context* const &ec);

        // 方便派生类覆盖 Close 函数:
        // if ... return false;
        // CloseChilds(reason);
        // 从容器变量移除
        // DelayUnhold();
        // return true;
        void CloseChilds(int const &reason, std::string_view const &desc);

    protected:
        friend Context;

        // 每帧 call cps Update, 清理超时握手数据
        void Timeout() override;
    };

    struct KcpPeer : Timer {
        // 用于收发数据的物理 udp peer( 后面根据标识来硬转为 Listener 或 Dialer )
        Shared<KcpBase> owner;
        // 对方的 addr( owner 收到数据时会填充. SendTo 时用到 )
        sockaddr_in6 addr{};
        // 收数据用堆积容器( Receive 里访问它来处理收到的数据 )
        Data recv;

        // kcp 相关上下文
        ikcpcb *kcp = nullptr;
        uint32_t conv = 0;
        int64_t createMS = 0;
        uint32_t nextUpdateMS = 0;

        // 初始化 kcp 相关上下文
        KcpPeer(Shared<KcpBase> const &owner, uint32_t const &conv);

        // 回收 kcp 相关上下文
        ~KcpPeer() override;

        // 被 ep 调用. 受帧循环驱动. 帧率越高, kcp 工作效果越好. 典型的频率为 100 fps
        void UpdateKcpLogic();

        // 被 owner 调用. 塞数据到 kcp
        void Input(uint8_t const *const &buf, size_t const &len, bool isFirst = false);

        // 回收 kcp 对象, 看情况从 ep->kcps 移除
        bool Close(int const &reason, std::string_view const &desc) override;

        // Close
        void Timeout() override;

        // 传进发送队列( 如果 !kcp 则忽略 )
        virtual int Send(uint8_t const *const &buf, size_t const &len);

        // 立刻开始发送
        virtual int Flush();

        // 数据接收事件: 用户可以从 recv 拿数据, 并移除掉已处理的部分
        virtual void Receive() = 0;

        // kcp != null && !owner
        bool Alive();
    };

    template<typename PeerType, class ENABLED = std::is_base_of<KcpPeer, PeerType>>
    struct KcpListener : KcpBase {
        // 默认握手超时时长
        int64_t shakeTimeoutMS = 3000;

        // 在构造里改点默认参数以令 udp peer 更适合做服务器端
        explicit KcpListener(Context* const &ec);

        // 1. 判断收到的数据内容, 模拟握手， 最后产生 KcpPeer
        // 2. 定位到 KcpPeer, Input 数据
        void Receive(uint8_t const *const &buf, size_t const &len) override;

        // 连接创建成功后会触发
        virtual void Accept(Shared<PeerType> const &peer) = 0;

        // 调用 CloseChilds
        bool Close(int const &reason, std::string_view const &desc) override;

        // MakeFD
        virtual int Listen(int const &port, char const* const& hostName = nullptr, bool const& reusePort = false, size_t const& rmem_max = 1784 * 5000, size_t const& wmem_max = 1784 * 5000);
    };






    /***********************************************************************************************************/
    // impls
    /***********************************************************************************************************/


    inline int UdpPeer::MakeFD(int const &port, char const* const& hostName, bool const& reusePort, size_t const& rmem_max, size_t const& wmem_max) {
        // 防重复 make
        if (this->fd != -1) return __LINE__;
        // 创建监听用 socket fd
        auto &&fd = ec->MakeSocketFD(port, SOCK_DGRAM, hostName, reusePort, rmem_max, wmem_max);
        if (fd < 0) return -1;
        // 确保 return 时自动 close
        auto sg = xx::MakeScopeGuard([&] { close(fd); });
        // fd 纳入 epoll 管理
        if (-1 == ec->Ctl(fd, EPOLLIN)) return -3;
        // 取消自动 close
        sg.Cancel();
        // 补映射( 因为 -1 调用的基类构造, 映射代码被跳过了 )
        this->fd = fd;
        ec->fdMappings[fd] = this;
        return 0;
    }


    inline bool KcpPeer::Alive() {
        return kcp != nullptr && owner;
    }

    inline KcpPeer::KcpPeer(Shared<KcpBase> const &owner, uint32_t const &conv) : Timer(owner->ec, -1) {
        // 创建并设置 kcp 的一些参数. 按照 每秒 100 帧来设置的. 即精度 10 ms
        kcp = ikcp_create(conv, this);
        (void) ikcp_wndsize(kcp, 1024, 1024);
        (void) ikcp_nodelay(kcp, 1, 10, 2, 1);
        kcp->rx_minrto = 10;
        kcp->stream = 1;
        //ikcp_setmtu(kcp, 470);    // 该参数或许能提速, 因为小包优先

        // 给 kcp 绑定 output 功能函数
        ikcp_setoutput(kcp, [](const char *inBuf, int len, ikcpcb *_, void *user) -> int {
            auto self = (KcpPeer *) user;
            // 如果物理 peer 没断, 就通过修改 addr 的方式告知 Send 函数目的地
            if (self->owner) {
                self->owner->addr = self->addr;
                return (int)self->owner->Send((uint8_t*)inBuf, len);
            }
            return -1;
        });

        // 填充初始参数
        this->owner = owner;
        this->conv = conv;
        this->createMS = ec->nowMS;
    }

    inline KcpPeer::~KcpPeer() {
        Close(-10, " KcpPeer ~KcpPeer");
    }

    inline void KcpPeer::Timeout() {
        Close(-11, " KcpPeer Timeout");
    }

    inline int KcpPeer::Send(uint8_t const *const &buf, size_t const &len) {
        if (!kcp) return -1;
        // 据kcp文档讲, 只要在等待期间发生了 ikcp_send, ikcp_input 就要重新 update
        nextUpdateMS = 0;
        return ikcp_send(kcp, (char*)buf, (int)len);
    }

    inline int KcpPeer::Flush() {
        if (!kcp) return __LINE__;
        ikcp_flush(kcp);
        return 0;
    }

    inline void KcpPeer::UpdateKcpLogic() {
        if (!kcp) throw std::runtime_error(" KcpPeer UpdateKcpLogic if (!kcp)");
        // 计算出当前 ms
        // 已知问题: 受 ikcp uint32 限制, 连接最多保持 50 多天
        auto &&currentMS = uint32_t(ec->nowMS - createMS);
        // 如果 update 时间没到 就退出
        if (nextUpdateMS > currentMS) return;
        // 来一发
        ikcp_update(kcp, currentMS);
        // 更新下次 update 时间
        nextUpdateMS = ikcp_check(kcp, currentMS);
    }

    inline void KcpPeer::Input(uint8_t const *const &buf, size_t const &len_, bool isFirst) {
        // 将底层数据灌入 kcp
        if (int r = ikcp_input(kcp, (char*)buf, (long)len_)) {
            Close(-12,
                  xx::ToString(__LINE__, " KcpPeer Input if (int r = ikcp_input(kcp, buf, len_)), r = ", r).c_str());
            return;
        }
        // 据kcp文档讲, 只要在等待期间发生了 ikcp_send, ikcp_input 就要重新 update
        nextUpdateMS = 0;
        // 开始处理收到的数据
        do {
            // 如果接收缓存没容量就扩容( 通常发生在首次使用时 )
            if (!recv.cap) {
                recv.Reserve(1024 * 256);
            }
            // 如果数据长度 == buf限长 就自杀( 未处理数据累计太多? )
            if (recv.len == recv.cap) {
                Close(-13, " KcpPeer Input if (recv.len == recv.cap)");
                return;
            }

            // 从 kcp 提取数据. 追加填充到 recv 后面区域. 返回填充长度. <= 0 则下次再说
            auto &&len = ikcp_recv(kcp, (char*)recv.buf + recv.len, (int) (recv.cap - recv.len));
            if (len <= 0) break;
            recv.len += len;

            // 如果是 5 字节 accept 首包，则忽略
            if (isFirst && recv.len >= 5 && *(uint32_t *) recv.buf == 1 && recv.buf[4] == 0) {
                recv.RemoveFront(5);
                if (!recv.len) continue;
            }

            // 调用用户数据处理函数
            Receive();
            // 如果用户那边已 Close 就直接退出
            if (!Alive()) return;
        } while (true);
    }

    inline bool KcpPeer::Close(int const &reason, std::string_view const &desc) {
        if (!kcp) return false;
        // 回收 kcp
        ikcp_release(kcp);
        kcp = nullptr;
        // 从父容器移除( 由父容器发起的 Close 会导致条件失败: 父容器调用 子Close 时先清 owner )
        if (owner) {
            owner->cps.erase(conv);
        }
        // 这里不负责自动 DelayUnhold. 如果有用到 Hold 机制，需要派生类自行处理
        return true;
    }

    inline KcpBase::KcpBase(Context* const &ec) : UdpPeer(ec, -1) {
        // 注册下一帧帧发起的回调( 回调中继续调用这句代码以实现每帧都产生回调 )
        SetTimeout(1);
    }



    ;
    template<typename PeerType, class ENABLED>
    inline KcpListener<PeerType, ENABLED>::KcpListener(Context* const &ec) : KcpBase(ec) {
        readCountAtOnce = 500;
    }

    template<typename PeerType, class ENABLED>
    inline bool KcpListener<PeerType, ENABLED>::Close(int const &reason, std::string_view const &desc) {
        // 防重入 顺便关 fd
        if (!this->KcpBase::Close(reason, desc)) return false;
        // 关闭所有虚拟 peer
        CloseChilds(reason, desc);
        // 从容器变量移除
        DelayUnhold();
        return true;
    }

    template<typename PeerType, class ENABLED>
    inline int KcpListener<PeerType, ENABLED>::Listen(int const &port, char const* const& hostName, bool const& reusePort, size_t const& rmem_max, size_t const& wmem_max) {
        return MakeFD(port, hostName, reusePort, rmem_max, wmem_max);
    }

    template<typename PeerType, class ENABLED>
    inline void KcpListener<PeerType, ENABLED>::Receive(uint8_t const *const &buf, size_t const &len) {
        // addr 转为 ip:port 当 key
        auto ip_port = xx::ToString(addr);

        // 当前握手方案为 Dialer 每秒 N 次不停发送 4 字节数据( serial )过来,
        // 收到后根据其 ip:port 做 key, 生成 convId. 每次收到都向其发送同样的 convId, 并续命
        if (len == 4) {
            auto &&iter = shakes.find(ip_port);
            if (iter == shakes.end()) {
                iter = shakes.emplace(ip_port, std::make_pair(++convId, ec->nowMS + shakeTimeoutMS)).first;
            }
            else {
                //iter->second.second = ec->nowMS + shakeTimeoutMS;
                iter.value().second = ec->nowMS + shakeTimeoutMS;
            }
            // 回发 serial + convId。客户端在收到回发数据后，会通过 kcp 发送 01 00 00 00 00 这样的 5 字节数据，以触发服务器 accept
            uint8_t tmp[8];
            memcpy(tmp, buf, 4);
            memcpy(tmp + 4, &iter->second.first, 4);
            Send(tmp, 8);
            return;
        }

        // 忽略长度小于 kcp 头的数据包 ( IKCP_OVERHEAD at ikcp.c )
        if (len < 24) return;

        // read conv header
        uint32_t conv;
        memcpy(&conv, buf, sizeof(conv));

        // 根据 conv 试定位到 peer
        auto &&peerIter = cps.find(conv);

        // 找到就灌入数据
        if (peerIter != cps.end()) {
            // 更新地址信息
            memcpy(&peerIter->second->addr, &addr, sizeof(addr));
            // 将数据灌入 kcp. 进而可能触发 peer->Receive 进而 Close
            peerIter->second->Input(buf, len);
        } else {
            // 如果不存在 就在 shakes 中按 ip:port 找
            auto &&iter = shakes.find(ip_port);
            // 未找到或 conv 对不上: 忽略
            if (iter == shakes.end() || iter->second.first != conv) return;

            // 从握手信息移除
            shakes.erase(iter);
            // 创建 peer
            auto &&peer = xx::Make<PeerType>(SharedFromThis(this), conv);
            // 放入容器( 这个容器不会加持 )
            cps[conv] = &*peer;
            // 更新地址信息
            memcpy(&peer->addr, &addr, sizeof(addr));
            // 触发事件回调
            Accept(peer);
            // 如果 已Close 或 未持有 就短路出去
            if (!peer->Alive() || peer.useCount() == 1) return;
            // 将数据灌入 kcp ( 可能继续触发 Receive 啥的 )
            peer->Input(buf, len, true);
        }
    }

    inline void KcpBase::Timeout() {
        // 更新所有 kcps
        for (auto &&kv : cps) {
            kv.second->UpdateKcpLogic();
        }
        // 清理超时握手信息
        for (auto &&iter = shakes.begin(); iter != shakes.end();) {
            if (iter->second.second < ec->nowMS) {
                iter = shakes.erase(iter);
            } else {
                ++iter;
            }
        }
        // 下帧继续触发
        SetTimeout(1);
    }

    inline void KcpBase::CloseChilds(int const &reason, std::string_view const &desc) {
        for (auto &&kv : cps) {
            // 先清掉 owner 避免 Close 函数内部到 cps 来移除自己, 同时减持父容器
            kv.second->owner.Reset();
            // 关掉 虚拟peer
            kv.second->Close(reason, desc);
        }
        // 减持所有 挂靠 peer
        cps.clear();
    }

    inline ssize_t UdpPeer::Send(uint8_t const *const &buf, size_t const &len) {
        return SendTo(addr, buf, len);
    }

    inline ssize_t UdpPeer::SendTo(sockaddr_in6 const& tarAddr, uint8_t const *const &buf, size_t const &len) {
        // 保底检查
        if (!Alive()) return -1;
        // 底层直发
        return sendto(fd, buf, len, 0, (sockaddr *) &tarAddr, sizeof(tarAddr));
    }

    inline void UdpPeer::EpollEvent(const uint32_t &e) {
        // fatal error
        if (e & EPOLLERR || e & EPOLLHUP) {
            Close(-14, " UdpPeer EpollEvent if (e & EPOLLERR || e & EPOLLHUP)");
            return;
        }
        // read
        if (e & EPOLLIN) {
            // 从 libuv 参考过来的经验循环( 32 ), 尽可能的读走数据
            for (int i = 0; i < readCountAtOnce; ++i) {
                socklen_t addrLen = sizeof(addr);
                ssize_t len;
                do {
                    len = recvfrom(fd, ec->buf.data(), ec->buf.size(), 0, (struct sockaddr *) &addr, &addrLen);
                }
                while (len == -1 && errno == EINTR);
                if (len == -1) {
                    auto er = errno;
                    if (er == EAGAIN || er == EWOULDBLOCK || er == ENOBUFS) return;
                    throw std::runtime_error(xx::ToString(__LINE__, " UdpPeer EpollEvent recvfrom rtv -1 errno = ", er));
                }
                // 可能收到 0 长度数据包. 或者没有发送地址的数据包. 忽略
                if (len == 0 || addrLen == 0) return;
                // 调用数据处理函数
                Receive(ec->buf.data(), len);
                // 可能被用户关闭
                if (!Alive()) return;
            }
        }
    }
}






/***********************************************************************************************************/
// 扩展 kcp dialer
/***********************************************************************************************************/

namespace xx::Epoll {

    // 这东西用起来要非常小心：闭包上下文可能加持指针，可能引用到野指针，可能析构对象导致 this 失效
    struct GenericTimer : Timer {
        using Timer::Timer;
        fu2::function<void()> onTimeout;

        inline void Timeout() override {
            onTimeout();
        }
    };

    template<typename PeerType, class ENABLED = std::is_base_of<KcpPeer, PeerType>>
    struct KcpDialer : KcpBase {
        // 自己本身的 timer 已在基类用于每帧更新 cps
        GenericTimer timerForShake;
        GenericTimer timerForTimeout;
        bool needSendFirstPackage = true;

        // 初始化 timer
        explicit KcpDialer(Context* const &ec, int const& port = 0);

        // 连接成功或失败(peer==nullptr)都会触发. 失败通常属于超时
        virtual void Connect(Shared<PeerType> const &peer) = 0;

        // 向 addrs 追加地址. 如果地址转换错误将返回非 0
        int AddAddress(std::string const &ip, int const &port);
        void AddAddress(sockaddr_in6 const& addr);

        // 开始拨号( 超时单位：帧 )。会遍历 addrs 为每个地址创建一个 TcpConn 连接
        // 保留先连接上的 socket fd, 创建 Peer 并触发 Connect 事件.
        // 如果超时，也触发 Connect，参数为 nullptr
        int Dial(int const &timeoutFrames);

        // 开始拨号( 超时单位：秒 )
        int DialSeconds(double const &timeoutSeconds);

        // 返回是否正在拨号
        bool Busy();

        // 停止拨号. 保留 addrs.
        void Stop();

    protected:
        // 存个空值备用 以方便返回引用
        Shared<PeerType> emptyPeer;

        // 1. 判断收到的数据内容, 模拟握手，最后产生 KcpPeer
        // 2. 定位到 KcpPeer, Input 数据( 已连接状态 )
        void Receive(uint8_t const *const &buf, size_t const &len) override;

        // 基于每个目标 addr 生成的 连接上下文对象，里面有 serial, conv 啥的. 拨号完毕后会被清空
        uint32_t serialAutoInc = 0;
        std::vector<uint32_t> serials;

        // 要连的地址数组
        std::vector<sockaddr_in6> addrs;

        // 关闭 fd, 关闭所有子
        bool Close(int const &reason, std::string_view const &desc) override;
    };

    template<typename PeerType, class ENABLED>
    void KcpDialer<PeerType, ENABLED>::Receive(uint8_t const *const &buf, size_t const &len) {
        if (len == 8) {
            // 收到 8 字节( serial + convId ) 握手回包
            // 如果没在拨号了，忽略
            if (serials.empty()) return;

            auto serial = *(uint32_t *) buf;
            auto convId = *(uint32_t *) (buf + 4);
            // 在拨号时产生的序号中查找
            size_t i = 0;
            for (; i < serials.size(); ++i) {
                if (serials[i] == serial) break;
            }
            // 找不到就忽略
            if (i == serials.size()) return;
            // 停止拨号
            Stop();
            // 创建 peer
            auto &&peer = xx::Make<PeerType>(SharedFromThis(this), convId);
            // 放入容器( 这个容器不会加持 )
            cps[convId] = &*peer;
            // 填充地址( 就填充拨号用的，不必理会收到的 )
            memcpy(&peer->addr, &addrs[i], sizeof(addr));
            // 如果需要发无意义首包刺激对方 accept. 如果是客户端先发数据 则不需要这样的首包
            if (needSendFirstPackage) {
                // 发 kcp 版握手包
                peer->KcpPeer::Send("\1\0\0\0\0", 5);
                peer->Flush();
            }
            // 触发事件回调
            Connect(peer);
        }
        else if (len < 24) {
            // 忽略长度小于 kcp 头的数据包 ( IKCP_OVERHEAD at ikcp.c )
            return;
        }
        else {
            // kcp 头 以 convId 打头
            auto convId = *(uint32_t *) buf;
            // 根据 conv 试定位到 peer. 找不到就忽略掉( 可能是上个连接的残留数据过来 )
            auto &&peerIter = cps.find(convId);
            // 找到就灌入数据
            if (peerIter != cps.end()) {
                // 将数据灌入 kcp. 进而可能触发 peer->Receive 进而可能 Close
                peerIter->second->Input(buf, len);
            }
        }
    }

    template<typename PeerType, class ENABLED>
    KcpDialer<PeerType, ENABLED>::KcpDialer(Context* const &ec, int const& port)
            : KcpBase(ec)
            , timerForShake(ec)
            , timerForTimeout(ec) {
        // 初始化握手 timer
        timerForShake.onTimeout = [this] {
            // 如果 serials 不空（ 意味着正在拨号 ), 就每秒数次无脑向目标地址发送 serial
            for (size_t i = 0; i < serials.size(); ++i) {
                SendTo(addrs[i], (uint8_t*)&serials[i], 4);
            }
            timerForShake.SetTimeoutSeconds(0.2);
        };

        // 初始化拨号超时 timer
        timerForTimeout.onTimeout = [this] {
            Stop();
            Connect(emptyPeer);
        };
    }

    template<typename PeerType, class ENABLED>
    void KcpDialer<PeerType, ENABLED>::Stop() {
        serials.clear();
        timerForShake.SetTimeout(0);
        timerForTimeout.SetTimeout(0);
    }

    template<typename PeerType, class ENABLED>
    bool KcpDialer<PeerType, ENABLED>::Busy() {
        return !serials.empty();
    }

    template<typename PeerType, class ENABLED>
    int KcpDialer<PeerType, ENABLED>::Dial(int const &timeoutFrames) {
        Stop();
        for (auto &&a : addrs) {
            serials.emplace_back(++serialAutoInc);
        }
        timerForShake.onTimeout();
        timerForTimeout.SetTimeout(timeoutFrames);
        return 0;
    }

    template<typename PeerType, class ENABLED>
    int KcpDialer<PeerType, ENABLED>::DialSeconds(double const &timeoutSeconds) {
        return Dial(ec->SecondsToFrames(timeoutSeconds));
    }

    template<typename PeerType, class ENABLED>
    int KcpDialer<PeerType, ENABLED>::AddAddress(std::string const &ip, int const &port) {
        auto &&a = addrs.emplace_back();
        if (int r = FillAddress(ip, port, a)) {
            addrs.pop_back();
            return r;
        }
        // todo: 去重
        return 0;
    }

    template<typename PeerType, class ENABLED>
    void KcpDialer<PeerType, ENABLED>::AddAddress(sockaddr_in6 const& addr) {
        // todo: 去重
        addrs.emplace_back(addr);
    }

    template<typename PeerType, class ENABLED>
    bool KcpDialer<PeerType, ENABLED>::Close(int const &reason, std::string_view const &desc) {
        // 防重入 顺便关 fd
        if (!this->KcpBase::Close(reason, desc)) return false;
        // 关闭所有虚拟 peer
        CloseChilds(reason, desc);
        // 从容器变量移除
        DelayUnhold();
        return true;
    }
}
