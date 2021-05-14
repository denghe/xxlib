﻿#pragma once
#include <ajson.hpp>
#include <iostream>

// 用类结构映射到 json 格式:

struct Config {
    uint32_t serverId = 0;
    uint32_t listenPort = 0;
    double peerTimeoutSeconds = 0;
    std::string dbIP;
    uint32_t dbPort = 0;
};
AJSON(Config, serverId, listenPort, peerTimeoutSeconds, dbIP, dbPort);


// 适配 std::cout
inline std::ostream& operator<<(std::ostream& o, Config const& c) {
    ajson::save_to(o, c);
    return o;
}

// 全局单例, 便于访问
inline Config config;
