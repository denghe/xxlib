#pragma once

#ifndef __ANDROID__
template<typename T> concept Has_GPeer_HandleCommand_Accept = requires(T t) { t.HandleCommand_Accept(std::string_view()); };
#else
template<class T, class = void> struct _Has_GPeer_HandleCommand_Accept : std::false_type {};
template<class T> struct _Has_GPeer_HandleCommand_Accept<T, std::void_t<decltype(std::declval<T&>().HandleCommand_Accept(std::string_view()))>> : std::true_type {};
template<class T> constexpr bool Has_GPeer_HandleCommand_Accept = _Has_GPeer_HandleCommand_Accept<T>::value;
#endif

// 来自网关的连接对端. 主要负责处理内部指令和 在 VPeer 之间 转发数据
template<typename PeerDeriveType, typename VPeerType, typename ServerType>
struct GPeerCode : xx::PeerCode<PeerDeriveType>, xx::PeerTimeoutCode<PeerDeriveType>, xx::PeerHandleMessageCode<PeerDeriveType> {
    using PC = xx::PeerCode<PeerDeriveType>;
    using PTC = xx::PeerTimeoutCode<PeerDeriveType>;

    ServerType& server;
    GPeerCode(ServerType& server_, asio::ip::tcp::socket&& socket_)
        : PC(server_.ioc, std::move(socket_))
        , PTC(server_.ioc)
        , server(server_) {
        PEERTHIS->ResetTimeout(15s);  // 设置初始的超时时长
    }
    uint32_t gatewayId = 0xFFFFFFFFu;	// gateway 连上来后应先发 gatewayId 内部指令, 存储在此，并放入 server.gpeers. 如果已存在，就掐线( 不顶下线 )
    std::unordered_map<uint32_t, std::shared_ptr<VPeerType>> vpeers;

    // PeerHandleMessageCode 会在包切片后调用
    // 根据 target 找到 VPeer 转发 或执行 内部指令, 非 0xFFFFFFFFu 则为正常数据，走转发流，否则走内部指令
    int HandleData(xx::Data_r&& dr) {
        uint32_t target;
        if (int r = dr.ReadFixed(target)) return __LINE__;		                    // 试读取 target. 失败直接断开
        if (target != 0xFFFFFFFFu) {
            //xx::CoutTN("HandleData target = ", target, " dr = ", dr);
            if (auto iter = vpeers.find(target); iter != vpeers.end() && iter->second->Alive()) {	//  试找到有效 vpeer 并转交数据
                iter->second->HandleData(std::move(dr));                            // 跳过 target 的处理
            }
        } else {	// 内部指令
            std::string_view cmd;
            if (int r = dr.Read(cmd)) return -__LINE__;                             // 试读出 cmd。出错返回负数，掐线
            if (cmd == "accept"sv) {                                                // gateway accept client socket
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;                    // 试读出 clientId。出错返回负数，掐线
                std::string_view ip;
                if (int r = dr.Read(ip)) return -__LINE__;                          // 试读出 ip。出错返回负数，掐线
                if constexpr (Has_GPeer_HandleCommand_Accept<PeerDeriveType>) {
                    if (int r = PEERTHIS->HandleCommand_Accept(ip)) return -__LINE__;   // ip 前置 check
                }
                if (auto p = CreateVPeer(clientId, ip)) {                           // 创建相应的 VPeer
                    PEERTHIS->Send(xx::MakeCommandData("open"sv, clientId));        // 下发 open 指令
                }
            } else if (cmd == "close"sv) {                                          // gateway client socket 已断开
                uint32_t clientId;
                if (int r = dr.Read(clientId)) return -__LINE__;
                xx::CoutTN("GPeer HandleData cmd = ", cmd, ", clientId = ", clientId);
                if (auto iter = vpeers.find(clientId); iter != vpeers.end()) {      // 找到就 Stop 并移除
                    iter->second->Stop();
                    vpeers.erase(iter);
                }
            } else if (cmd == "ping"sv) {                                           // 来自 网关 的 ping. 直接 echo 回去
                //xx::CoutTN("HandleData cmd = ", cmd);
                PEERTHIS->Send(dr);
            } else if (cmd == "gatewayId"sv) {                                      // gateway 连上之后的首包, 注册自己
                if (gatewayId != 0xFFFFFFFFu) {                                     // 如果不是初始值, 说明收到过了
                    xx::CoutTN("GPeer HandleData dupelicate cmd = ", cmd);
                    return -__LINE__;
                }
                if (int r = dr.Read(gatewayId)) return -__LINE__;
                xx::CoutTN("GPeer HandleData cmd = ", cmd, ", gatewayId = ", gatewayId);
                if (auto iter = server.gpeers.find(gatewayId); iter != server.gpeers.end()) return -__LINE__;   // 相同id已存在：掐线
                server.gpeers[gatewayId] = PEERTHIS->shared_from_this();            // 放入容器备用
            } else {
                std::cout << "GPeer HandleData unknown cmd = " << cmd << std::endl;
            }
        }
        PEERTHIS->ResetTimeout(15s);                                                // 无脑续命
        return 0;
    }

    void Stop_() {
        for (auto& kv : vpeers) {
            kv.second->Stop();                                                      // 停止所有虚拟peer
        }
        vpeers.clear(); // 根据业务需求来。有可能 vpeer 在 stop 之后还会保持一段时间，甚至 重新激活
        server.gpeers.erase(gatewayId);                                             // 从容器移除
    }

    // 创建并插入 vpeers 一个 VPeer. 接着将执行其 Start_ 函数
    // 常用于服务之间 通知对方 通过 gatewayId + clientId 创建一个 vpeer 并继续后续流程. 比如 通知 游戏服务 某网关某玩家 要进入
    std::shared_ptr<VPeerType> CreateVPeer(uint32_t const& clientId, std::string_view const& ip) {
        if (vpeers.find(clientId) != vpeers.end()) return {};
        auto& p = vpeers[clientId];
        p = std::make_shared<VPeerType>(server, *PEERTHIS, clientId, ip);           // 放入容器备用
        p->Start();
        return p;
    }
};
