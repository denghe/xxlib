#include "server.h"
#include "peer.h"
#include "config.h"
#include "listener.h"
#include "xx_logger.h"
#include "db.h"

int Server::Init() {
    // 初始化监听器
    xx::MakeTo(listener, this);

    // 如果监听失败则输出错误提示并退出
    if (int r = listener->Listen((int)config.listenPort)) {
        LOG_ERROR("listen to port ", config.listenPort, "failed.");
        return r;
    }

    // init db
    xx::MakeTo(db, this);

    // 注册交互指令
    EnableCommandLine();

    cmds["?"] = [this](auto args) {
        std::cout << "cmds: cfg, info, quit / exit" << std::endl;
    };
    cmds["c"] = [this](auto args) {
        std::cout << "cfg = " << config << std::endl;
    };
//    cmds["i"] = [this](auto args) {
//        std::cout << GetInfo() << std::endl;
//    };
    cmds["q"] = [this](auto args) {
        running = false;
    };
    return 0;
}

