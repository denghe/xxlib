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

    // init game context & cache
    scene.Emplace()->shooter.Emplace();
    sync.Emplace();
    sync->scene = scene;
    WriteTo(syncData, sync);

    // store singleton instance for SS visit
    instance = this;
    return 0;
}

int Server::FrameUpdate() {
    // frame limit to 60 fps
    totalDelta += (double) (nowMS - lastMS) / 1000.;
    lastMS = nowMS;
    while (totalDelta > (1.f / 60.f)) {
        totalDelta -= (1.f / 60.f);
        // fill syncData
        WriteTo(syncData, sync);
        // handle packages
        for (auto &kv : ps) {
            auto &p = *kv.second;
            auto &recvs = p.recvs;
            // every time handle 1 package
            if (!recvs.empty()) {

                auto &o = recvs.front();
                switch (o.typeId()) {
                    case xx::TypeId_v<SS_C2S::Cmd>: {
                        auto &m = o.ReinterpretCast<SS_C2S::Cmd>();
                        scene->shooter->cs = m->cs;
                        break;
                    }
                }
                recvs.pop();
            }
        }
        if (int r = scene->Update()) {
            // todo
            break;
        }
    }
    return 0;
}

int Server::HandlePackage(Peer &p, xx::ObjBase_s &o) {
    switch (o.typeId()) {
        case xx::TypeId_v<SS_C2S::Enter>: {
            p.Send(syncData);
            return 0;
        }
        case xx::TypeId_v<SS_C2S::Cmd>: {
            p.recvs.push(std::move(o));
            if (p.recvs.size() > 30) return -2;
            return 0;
        }
    }
    return -1;
}

void Server::WriteTo(xx::Data &d, xx::ObjBase_s const &o) {
    d.Clear();
    d.Reserve(8192);
    auto bak = d.WriteJump<false>(sizeof(uint32_t));
    om.WriteTo(d, o);
    d.WriteFixedAt(bak, (uint32_t) (d.len - sizeof(uint32_t)));
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