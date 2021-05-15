﻿#include "server.h"
#include "config.h"
#include "glistener.h"
#include "timer.h"
#include "xx_logger.h"
#include "gpeer.h"
#include "vpeer.h"
#include "dbpeer.h"
#include "dbdialer.h"
#include "lpeer.h"
#include "ldialer.h"

int Server::Init() {
    // 初始化监听器
    xx::MakeTo(gatewayListener, this);

    // 如果监听失败则输出错误提示并退出
    if (int r = gatewayListener->Listen((int)config.listenPort)) {
        LOG_ERROR("listen to port ", config.listenPort, "failed.");
        return r;
    }

    // init db dialer
    xx::MakeTo(dbDialer, this);
    dbDialer->AddAddress(config.dbIP, (int)config.dbPort);

    // init lobby dialer
    xx::MakeTo(lobbyDialer, this);
    lobbyDialer->AddAddress(config.lobbyIP, (int)config.lobbyPort);

    // 初始化间隔时间为 ? 秒的处理服务器之间 ping 防止连接僵死的 timer
    xx::MakeTo(timer, this);
    timer->Start();

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