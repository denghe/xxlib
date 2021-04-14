﻿#include "server.h"
#include "dialer.h"
#include "config.h"
#include "listener.h"
#include "pingtimer.h"
#include "tasktimer.h"
#include "xx_logger.h"

Server::~Server() {
    DisableCommandLine();
    listener.Reset();
    pingTimer.Reset();
    taskTimer.Reset();

    for(auto&& dp : dps) {
        dp.second.first->Stop();
        if (dp.second.second) {
            dp.second.second->Close(-__LINE__, " Server Run sg1");
        }
    }
    dps.clear();

    std::vector<uint32_t> keys;
    for(auto&& cp : cps) {
        keys.push_back(cp.first);
    }
    for(auto&& key : keys) {
        cps[key]->Close(-__LINE__, " Server Run sg1");
    }
    cps.clear();

    holdItems.clear();

    assert(xx::SharedFromThis(this).useCount() == 2);
}

int Server::Init() {
    // 初始化监听器和回收sg
    xx::MakeTo(listener, xx::SharedFromThis(this));
    // 如果监听失败则输出错误提示并退出
    if (int r = listener->Listen((int)config.listenPort)) {
        LOG_ERROR("listen to port ", config.listenPort, "failed.");
        return r;
    }

    // 初始化间隔时间为 ? 秒的处理服务器之间 ping 防止连接僵死的 timer
    xx::MakeTo(pingTimer, xx::SharedFromThis(this));
    pingTimer->Start();

    // 初始化间隔时间为 1 秒的处理服务器之间断线重拨的 timer
    xx::MakeTo(taskTimer, xx::SharedFromThis(this));
    taskTimer->Start();

    // 遍历配置并生成相应的 dialer
    for (auto &&si : config.serverInfos) {
        // 创建拨号器
        auto&& dialer = xx::MakeShared<Dialer>(xx::SharedFromThis(this));
        // 放入字典。如果 server id 重复就报错
        if (!dps.insert({si.serverId, std::make_pair(dialer, nullptr)}).second) {
            LOG_ERROR("duplicate serverId: ", si.serverId);
            return __LINE__;
        }
        // 填充数据，为开始拨号作准备（会在帧回调逻辑中开始拨号）
        dialer->serverId = (int)si.serverId;
        dialer->AddAddress(si.ip, (int)si.port);
    }

    // 核查是否存在 0 号服务的 dialer. 没有就报错
    if (dps.find(0) == dps.end()) {
        LOG_ERROR("can't find base server ( serverId = 0 )'s dialer.");
        return __LINE__;
    }

    // 注册交互指令
    EnableCommandLine();

    cmds["?"] = [this](auto args) {
        std::cout << "cmds: cfg, info, quit / exit" << std::endl;
    };
    cmds["cfg"] = [this](auto args) {
        std::cout << "cfg = " << config << std::endl;
    };
    cmds["info"] = [this](auto args) {
        std::cout << GetInfo() << std::endl;
    };
    cmds["quit"] = [this](auto args) {
        running = false;
    };
    cmds["exit"] = this->cmds["quit"];

    return 0;
}

int Server::FrameUpdate() {
    return 0;
}

std::string Server::GetInfo() {
    std::string s;
    xx::Append(s, "dps.size() = ", dps.size()
            , "  [[\"serverId\",\"ip:port\",\"busy\",\"peer alive\",\"ping\"]");
    for (auto &&kv : dps) {
        auto &&dialer = kv.second.first;
        auto &&peer = kv.second.second;
        xx::Append(s,",[", dialer->serverId
                , ",", xx::ToString(dialer->addrs[0])
                , ",", (dialer->Busy() ? "true" : "false")
                , ",", (peer ? "true" : "false")
                , ",", (peer ? peer->pingMS : 0), "]");
    }
    xx::Append(s,"]    ", "cps.size() = ", cps.size()
            , "  [[\"clientId\",\"ip:port\"]");
    for (auto &&kv : cps) {
        xx::Append(s,",[", kv.first, ",\"", kv.second->addr,"\"]");
    }
    xx::Append(s,"]  ");
    return s;
}
