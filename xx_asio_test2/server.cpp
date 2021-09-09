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
    scene->shooter->scene = scene;
    sync.Emplace();
    sync->scene = scene;
    WriteTo(syncData, sync);
    event.Emplace();
    totalDelta = 0;
    lastMS = nowMS;

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

        // cs store
        auto cs = scene->shooter->cs;

        // handle packages( change cs )
        for (auto &kv : ps) {
            auto &p = *kv.second;
            auto &recvs = p.recvs;
            // every time handle 1 package
            while (!recvs.empty()) {
                auto &o = recvs.front();
                switch (o.typeId()) {
                    case xx::TypeId_v<SS_C2S::Enter>: {
                        // enter should be the first package
                        if (p.synced) break;    // todo: error handle
                        p.synced = true;
                        if (!syncData.len) {
                            WriteTo(syncData, sync);
                        }
                        p.Send(syncData);
                        p.SetTimeoutSeconds(15);
                        LOG_INFO("clientId = ", p.clientId, " recv Enter: ", om.ToString(o));
                        break;
                    }
                    case xx::TypeId_v<SS_C2S::Cmd>: {
                        // cmd should be after enter
                        if (!p.synced) break;   // todo: error handle
                        auto &m = o.ReinterpretCast<SS_C2S::Cmd>();
                        cs = m->cs;
                        p.SetTimeoutSeconds(15);
                        LOG_INFO("clientId = ", p.clientId, " recv Cmd: ", om.ToString(m));
                        break;
                    }
                }
                recvs.pop();
            }
        }

        // if cs changed, send event
        if (scene->shooter->cs != cs) {
            scene->shooter->cs = cs;
            event->cs = cs;
            event->frameNumber = scene->frameNumber;
            WriteTo(eventData, event);
            for(auto& kv : ps) {
                kv.second->Send(eventData);
            }
        }

        // logic update
        if (int r = scene->Update()) {
            // todo: game over?
            break;
        }

        // clear sync cache
        syncData.Clear();
    }
    return 0;
}

int Server::HandlePackage(Peer &p, xx::ObjBase_s &o) {
    switch (o.typeId()) {
        case xx::TypeId_v<SS_C2S::Enter>:
        case xx::TypeId_v<SS_C2S::Cmd>:
            p.recvs.push(std::move(o));
            if (p.recvs.size() > 30) return -2;
            return 0;
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
