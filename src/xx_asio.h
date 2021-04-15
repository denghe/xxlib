#pragma once

#include "xx_helpers.h"
#include "xx_string.h"
#include <asio.hpp>
#include "ikcp.h"
#include "xx_ptr.h"
#include <sstream>
#include <deque>

// 为 cocos / unity 之类客户端 实现一个带 域名解析, kcp拨号 与 通信 的 网关拨号版本。功能函数和用法，与 xx::UvClient 封装的 lua 接口类似

// void SetDomainPort(string domainName, int port)              -- 设置 域名/ip 和 端口

// void Update())                                               -- 每帧来一发

// int Resolve()                                                -- 开始域名解析
// bool IPListIsEmpty()                                         -- 判断解析后的域名列表是否为空( 空则解析失败 )
// string[] GetIPList()                                         -- 获取解析后的域名列表 for dump

// int Dial()                                                   -- 拨号
// bool Busy())                                                 -- 是否正在解析或拨号
// void Cancel())                                               -- 取消解析或拨号

// void Disconnect())                                           -- 断开连接( 清除连接 )
// bool Alive()                                                 -- 连接是否存在( 因为是 udp, 故不知道是否断开 )
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

    struct Client {
        Client(Client const &) = delete;

        Client &operator=(Client const &) = delete;

        asio::io_context ioc;
        asio::ip::udp::resolver resolver;
        std::vector<asio::ip::address> addrs;
        bool resolving = false;
        int port = 0;
        std::vector<Shared<KcpPeer>> dials;
        Shared<KcpPeer> peer;

        Client() : ioc(1), resolver(ioc) {}
        void Update();

        inline int Resolve(std::string const &domain) {
            resolver.cancel();
            addrs.clear();
            resolving = true;
            resolver.async_resolve(domain, "", [this, domain](const asio::error_code &error, asio::ip::udp::resolver::results_type const &results) {
                resolving = false;
                if (!error.value()) {
                    for (auto &&r : results) {
                        addrs.emplace_back(r.endpoint().address());
                    }
                }
            });
            return 0;
        }

        inline void Cancel() {
            resolver.cancel();
            resolving = false;
            dials.clear();
        }

        [[nodiscard]] inline bool Busy() const {
            return resolving || !dials.empty();
        }

        inline void SetPort(int const &port_) {
            port = port_;
        }

        [[nodiscard]] inline std::vector<std::string> GetIPList() const {
            std::vector<std::string> r;
            for (auto &a : addrs) {
                xx::Append(r.emplace_back(), a);
            }
            return r;
        }
    };

    struct KcpPeer {
        Client *client;
        int closed = 0;
        std::string closedDesc;
        asio::ip::udp::socket socket;
        asio::ip::udp::endpoint ep;
        char udpRecvBuf[1024 * 64];
        ikcpcb *kcp = nullptr;
        uint32_t conv = 0;
        uint32_t shakeSerial = 0;
        int64_t createMS = 0;
        Data recv;
        std::deque<Shared<Data>> recvs; // for TryGetPackage

        // 初始化 sokcet
        explicit KcpPeer(Client *const &c, asio::ip::udp::endpoint const &ep, uint32_t const &shakeSerial)
                : client(c), socket(c->ioc, asio::ip::udp::endpoint(ep.protocol(), 0)), ep(ep), shakeSerial(shakeSerial) {
            socket.non_blocking(true);
            recv.Reserve(1024 * 256);
        }

        // 回收 kcp
        ~KcpPeer() {
            Close(-__LINE__, "~KcpPeer()");
        }

        inline void Update() {
            if (closed) return;
            asio::error_code e;
            asio::ip::udp::endpoint p;
            if (auto recvLen = socket.receive_from(asio::buffer(udpRecvBuf), p, 0, e)) {
                RecvHandler(e, recvLen);
            }
            ikcp_update(kcp, (IUINT32) (NowSteadyEpochMilliseconds() - createMS));
        }

        // udp 接收回调。kcp input 或 握手
        inline void RecvHandler(asio::error_code const &e, size_t recvLen) {
            if (closed) return;
            if (kcp) {
                // 准备向 kcp 灌数据并收包放入 recvs
                // 前置检查. 如果数据长度不足( kcp header ), 或 conv 对不上就 忽略
                if (recvLen < 24 || conv != *(uint32_t *) (udpRecvBuf)) return;

                // 将数据灌入 kcp. 灌入出错则 Close
                if (int r = ikcp_input(kcp, udpRecvBuf, (long) recvLen)) {
                    Close(r, "ikcp_input");
                    return;
                }

                // 开始处理收到的数据
                do {
                    // 如果数据长度 == buf限长 就自杀( 未处理数据累计太多? )
                    if (recv.len == recv.cap) {
                        Close(-__LINE__, "recv.len == recv.cap");
                        return;
                    }

                    // 从 kcp 提取数据. 追加填充到 recv 后面区域. 返回填充长度. <= 0 则下次再说
                    auto &&len = ikcp_recv(kcp, (char*)recv.buf + recv.len, (int) (recv.cap - recv.len));
                    if (len <= 0) return;
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

                        // 复制包到 recvs
                        recvs.emplace_back(Make<Data>(buf, totalLen));

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

                    // 记录创建毫秒数 for Update
                    createMS = NowSteadyEpochMilliseconds();

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
                    Send("\1\0\0\0\0", 5);
                    Flush();

                    // 将自己放入 client peer 并停止拨号
                    client->peer = SharedFromThis(this);
                    client->Cancel();
                }
            }
        }

        inline int Send(char const *const &buf, size_t const &len) {
            if (closed) return -1;
            return ikcp_send(kcp, buf, (int) len);
        }

        // 立刻开始发送
        inline void Flush() {
            if (closed) return;
            ikcp_flush(kcp);
        }


        // 回收 kcp 并清空数据, 撤销超时回调
        bool Close(int const &reason, char const *const &desc) {
            if (closed) return false;
            if (kcp) {
                ikcp_release(kcp);
                kcp = nullptr;
            }
            recvs.clear();
            closed = reason;
            closedDesc = desc;
            return true;
        }
    };

    inline void Client::Update() {
        if (ioc.stopped()) {
            ioc.restart();
        }
        ioc.poll();
        for (auto &&dp : dials) {
            dp->Update();
        }
        if (peer) {
            peer->Update();
        }
    }
}

//namespace xx::Asio {
//	using namespace std::placeholders;
//
//	struct KcpPeer;
//
//	// 可域名解析，可拨号，最后访问其 peer 成员通信
//	struct Client {
//        Client() = default;
//		Client(Client const&) = delete;
//		Client& operator=(Client const&) = delete;
//
//		// 被 cocos Update( delta ) 调用 以处理网络事件
//		int RunOnce(double const& delta);
//
//		// 可临时存点啥
//		void* userData = nullptr;
//		size_t userSize = 0;
//		int userInt = 0;
//
//		// asio 上下文
//		asio::io_context ioc;
//
//		// 域名 -- 地址列表 映射
//		std::unordered_map<std::string, std::vector<asio::ip::address>> domainAddrs;
//
//		// 域名 -- 解析器 映射. 如果不空, 就是有正在解析的
//		std::unordered_map<std::string, asio::ip::udp::resolver> domainResolvers;
//
//		// 域名解析
//		inline int ResolveDomain(std::string const& domain) {
//			auto&& r = domainResolvers.emplace(domain, asio::ip::udp::resolver(ioc));
//			if (r.second) {
//				r.first->second.async_resolve(domain, "", [this, domain](const asio::error_code& error, asio::ip::udp::resolver::results_type results) {
//					if (!error.value()) {
//						auto&& as = domainAddrs[domain];
//						as.clear();
//						for (auto&& r : results) {
//							as.push_back(r.endpoint().address());
//						}
//					}
//					domainResolvers.erase(domain);
//					});
//				return 0;
//			}
//			// 域名正在解析中
//			return -1;
//		}
//
//		// 打印解析出来的域名
//		inline void DumpDomainAddrs() {
//			for (auto&& kv : domainAddrs) {
//				std::cout << "domain = \"" << kv.first << "\", ip list = {" << std::endl;
//				for (auto&& a : kv.second) {
//					std::cout << "    " << a << std::endl;
//				}
//				std::cout << "}" << std::endl;
//			}
//		}
//
//		// 要 dial 的目标地址列表( 带端口 )
//		std::unordered_map<asio::ip::address, std::vector<uint16_t>> dialAddrs;
//
//		// 添加拨号地址. 端口合并去重
//		inline void AddDialAddress(asio::ip::address const& a, std::initializer_list<uint16_t> ports_) {
//			auto&& ps = dialAddrs[a];
//			ps.insert(ps.end(), ports_);
//			ps.erase(std::unique(ps.begin(), ps.end()), ps.end());
//		}
//
//		// 从 domainAddrs 拿域名对应的 ip 列表, 附加多个端口后放入拨号地址集合
//		inline int AddDialDomain(std::string const& domain, std::initializer_list<uint16_t> ports_) {
//			auto&& iter = domainAddrs.find(domain);
//			if (iter == domainAddrs.end()) return -2;
//			for (auto&& a : iter->second) {
//				AddDialAddress(a, ports_);
//			}
//			return 0;
//		}
//
//		// 添加拨号地址( string 版 ). 端口合并去重
//		inline void AddDialIP(std::string const& ip, std::initializer_list<uint16_t> ports_) {
//			AddDialAddress(asio::ip::address::from_string(ip), ports_);
//		}
//
//		// 正在拨号的 peers 队列. 如果正在拨号，则该队列不空. 每 FrameUpdate 将驱动其逻辑
//		std::vector<std::shared_ptr<KcpPeer>> dialPeers;
//
//		// 握手用 timer
//		xx::Looper::TimerEx shakeTimer;
//
//		// 拨号超时检测用 timer
//		xx::Looper::TimerEx dialTimer;
//
//		// 自增以产生握手用序列号
//		uint32_t shakeSerial = 0;
//
//		// 根据 dialAddrs 的配置，对这些 ip:port 同时发起拨号( 带超时 ), 最先连上的存到 peer 并停止所有拨号
//		void Dial(float const& timeoutSeconds);
//
//		// 停止拨号
//		void Stop();
//
//		// 返回是否正在拨号
//		inline bool Busy() { return !dialPeers.empty(); }
//
//		// 当前 peer( 拨号成功将赋值 )
//		std::shared_ptr<KcpPeer> peer;
//
//		bool PeerAlive();
//	};
//

//
//	inline void Client::Dial(float const& timeoutSeconds) {
//		Stop();
//		for (auto&& kv : dialAddrs) {
//			for (auto&& port : kv.second) {
//				dialPeers.emplace_back(Make<KcpPeer>(this, asio::ip::udp::endpoint(kv.first, port), ++shakeSerial));
//			}
//		}
//		// 启动拨号超时检测 timer
//		dialTimer.SetTimeout(timeoutSeconds);
//
//		// 启动握手 timer 并立刻发送握手包
//		shakeTimer.onTimeout(&shakeTimer);
//	}
//
//	inline void Client::Stop() {
//		dialPeers.clear();
//		shakeTimer.ClearTimeout();
//		dialTimer.ClearTimeout();
//	}
//
//	inline int Client::FrameUpdate() {
//		if (peer) {
//			peer->Update();
//		}
//		return this->BaseType::FrameUpdate();
//	}
//
//	inline Client::Client(size_t const& wheelLen, double const& frameRate_)
//		: BaseType(wheelLen, frameRate_)
//		, shakeTimer(this)
//		, dialTimer(this)
//	{
//		// 设置握手回调: 不停的发送握手包
//		shakeTimer.onTimeout = [this](auto t) {
//			for (auto&& p : dialPeers) {
//				try {
//					p->socket.send_to(asio::buffer(&p->shakeSerial, sizeof(p->shakeSerial)), p->ep);
//				}
//				catch (...) {
//
//				}
//			}
//			t->SetTimeout(0.2);
//		};
//
//		// 设置拨号回调: 到时就 Stop
//		dialTimer.onTimeout = [this](auto t) {
//			Stop();
//		};
//	}
//
//	inline bool Client::PeerAlive() {
//		return peer && !peer->closed;
//	}
//
//	inline int Client::RunOnce(double const& elapsedSeconds) {
//		ioc.poll();
//		if (Busy()) {
//			for (auto&& dp : dialPeers) {
//				dp->Update();
//			}
//		}
//		if (PeerAlive()) {
//			peer->Update();
//		}
//		secondsPool += elapsedSeconds;
//		auto rtv = this->BaseType::RunOnce();
//		ioc.poll();
//		return rtv;
//	}
//}



//// 适配 ip 地址做 map key
//namespace std {
//	template <>
//	struct hash<asio::ip::address> {
//		inline size_t operator()(asio::ip::address const& v) const {
//			if (v.is_v4()) return v.to_v4().to_ulong();
//			else if (v.is_v6()) {
//				auto bytes = v.to_v6().to_bytes();
//				auto&& p = (uint64_t*)&bytes;
//				return p[0] ^ p[1] ^ p[2] ^ p[3];
//			}
//			else return 0;
//		}
//	};
//}
