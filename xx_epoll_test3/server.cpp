#include "server.h"
#include "config.h"
#include "listener.h"
#include "pingtimer.h"
#include "xx_logger.h"
#include "apeer.h"
// todo: include speer gpeer vpeer ...

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

std::string Server::GetInfo() {
    std::string s;
//    xx::Append(s, "dps.size() = ", dps.size()
//            , "  [[\"serverId\",\"ip:port\",\"busy\",\"peer alive\",\"ping\"]");
//    for (auto &&kv : dps) {
//        auto &&dialer = kv.second.first;
//        auto &&peer = kv.second.second;
//        xx::Append(s,",[", dialer->serverId
//                , ",", xx::ToString(dialer->addrs[0])
//                , ",", (dialer->Busy() ? "true" : "false")
//                , ",", (peer ? "true" : "false")
//                , ",", (peer ? peer->pingMS : 0), "]");
//    }
//    xx::Append(s,"]    ", "cps.size() = ", cps.size()
//            , "  [[\"clientId\",\"ip:port\"]");
//    for (auto &&kv : cps) {
//        xx::Append(s,",[", kv.first, ",\"", kv.second->addr,"\"]");
//    }
//    xx::Append(s,"]  ");
    return s;
}
