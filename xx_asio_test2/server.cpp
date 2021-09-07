#include "server.h"
#include "listener.h"
#include "xx_logger.h"

int Server::Init() {
    // kcp 需要更高帧率运行以提供及时的响应
    SetFrameRate(100);

    // 初始化监听器
    xx::MakeTo(listener, this);
    // 如果监听失败则输出错误提示并退出
    if (int r = listener->Listen(12345)) {
        LOG_ERROR("listen to port 12345 failed.");
        return r;
    }

    return 0;
}

int Server::FrameUpdate() {
    //xx::CoutN(".");
    return 0;
}
