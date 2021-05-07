﻿#include "server.h"
#include "config.h"
#include "listener.h"
#include "pingtimer.h"
#include "xx_logger.h"
#include "apeer.h"
#include "gpeer.h"
#include "vpeer.h"

int Server::Init() {
    // 初始化监听器
    xx::MakeTo(listener, this);

    // 如果监听失败则输出错误提示并退出
    if (int r = listener->Listen((int)config.listenPort)) {
        LOG_ERROR("listen to port ", config.listenPort, "failed.");
        return r;
    }

    // 初始化间隔时间为 ? 秒的处理服务器之间 ping 防止连接僵死的 timer
    xx::MakeTo(pingTimer, this);
    pingTimer->Start();

    // set 10 times per seconds for FrameUpdate
    SetFrameRate(10);

    // 注册交互指令
    EnableCommandLine();

    cmds["?"] = [this](auto args) {
        std::cout << "cmds: cfg, info, quit / exit" << std::endl;
    };
    cmds["c"] = [this](auto args) {
        std::cout << "cfg = " << config << std::endl;
    };
    cmds["i"] = [this](auto args) {
        std::cout << GetInfo() << std::endl;
    };
    cmds["q"] = [this](auto args) {
        running = false;
    };
    return 0;
}

std::string Server::GetInfo() {
    std::string s;
    xx::Append(s, "gps.size() = ", gps.size(), "\r\n");
    for (auto &&kv : gps) {
        xx::Append(s,"gatewayId:", kv.first, ", ip:", kv.second->addr, "\r\n");
    }
    xx::Append(s, "\r\nvps.size() = ", vps.Count(), "\r\n");
    for (auto &&node : vps.dict) {
        xx::Append(s, "gatewayId:", (node.value->gatewayPeer? -1: node.value->gatewayPeer->gatewayId)
        , ", clientId: ", node.value->clientId
        , ", accountId: ", node.value->accountId
        );
    }
    return s;
}

int Server::FrameUpdate() {
    // logic drive
    auto dt = (double)nowMS / 1000;
    for (auto &&node : vps.dict) {
        node.value->Update(dt);
    }
    return 0;
}
