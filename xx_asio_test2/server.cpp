#include "server.h"
#include "listener.h"
#include "xx_logger.h"
#include "ss.h"

int Server::Init() {
    // kcp 需要高帧率运行以提供及时的响应
    SetFrameRate(100);

    // 初始化监听器
    xx::MakeTo(listener, this);
    // 如果监听失败则输出错误提示并退出
    if (int r = listener->Listen(12345)) {
        LOG_ERROR("listen to port 12345 failed.");
        return r;
    }

    // init game context
    auto scene = xx::Make<SS::Scene>();
    gctx = scene;
    // init shooter
    auto &&shooter = scene->shooter.Emplace();

    return 0;
}

int Server::FrameUpdate() {
    // todo: frame limit to ??? call update, every frame send events

    auto &&scene = gctx.ReinterpretCast<SS::Scene>();
    if (int r = scene->shooter->Update()) {
        // todo: dispose shooter?
    }

//    // every 1 seconds timer
//    if (nowMS > lastMS) {
//        lastMS = nowMS + 1000;
//        for(auto& kv : ps) {
//            auto& p = *kv.second;
//            assert(p.Alive());
//            auto o = xx::Make<SS_S2C::Event>();
//            p.Send(o);
//        }
//    }
    //xx::CoutN(".");
    return 0;
}
