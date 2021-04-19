#pragma once
#include <ajson.hpp>
#include <iostream>

// 用类结构映射到 json 格式:

struct ServerInfo {
    uint32_t serverId = 0;                      // 服务id（连上之后校验用）
    std::string ip;
    uint32_t port = 0;
};
AJSON(ServerInfo, serverId, ip, port);

struct Config {
    uint32_t gatewayId = 0;						// 网关id, 各个网关之间不可重复. 可以和 serverId 重复
    double clientTimeoutSeconds = 0;            // 客户端连接如果超过这个时间没有流量产生则被踢
    uint32_t listenPort = 0;					// 监听端口
    std::vector<ServerInfo> serverInfos;	    // 要连接到哪些服务
};
AJSON(Config, gatewayId, clientTimeoutSeconds, listenPort, serverInfos);


// 适配 std::cout
inline std::ostream& operator<<(std::ostream& o, Config const& c) {
    ajson::save_to(o, c);
    return o;
}

// 全局单例, 便于访问
inline Config config;
