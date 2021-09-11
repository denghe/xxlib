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
    scene.Emplace();
    sync.Emplace();
    event.Emplace();
    enterResult.Emplace();
    sync->scene = scene;
    totalDelta = 0;
    lastMS = nowMS;

    // store singleton instance for SS visit
    instance = this;

    return 0;
}

int Server::FrameUpdate() {
    // frame limit
    totalDelta += (double) (nowMS - lastMS) / 1000.;
    lastMS = nowMS;
    while (totalDelta > (1.f / 60.f)) {
        totalDelta -= (1.f / 60.f);

        // send sync to new enters
        if (!newEnters.empty()) {
            WriteTo(syncData, sync);
            for (auto &kv : newEnters) {
                assert(ps.contains(kv.first));
                auto &p = ps[kv.first];
                p->Send(syncData);
                break;
            }
        }

        // handle new enters
        for (auto &kv : newEnters) {
            // create shooter
            auto &&shooter = scene->shooters[kv.first].Emplace();
            shooter->scene = scene;
            shooter->clientId = kv.first;
            // todo: set random pos & angle

            // make event
            event->enters.push_back(shooter);
        }

        // handle shooters cs
        for (auto &kv : ps) {
            auto &rs = kv.second->recvs;
            // every time handle 1 package
            if (!rs.empty()) {
                auto &o = rs.front();
                assert(o.typeId() == xx::TypeId_v<SS_C2S::Cmd>);
                auto &m = o.ReinterpretCast<SS_C2S::Cmd>();

                // find shooter
                assert(scene->shooters.contains(kv.first));
                auto &shooter = scene->shooters[kv.first];

                // make event if cs changed
                if (shooter->cs != m->cs) {
                    shooter->cs = m->cs;
                    event->css.emplace_back(kv.first, m->cs);
                }
                rs.pop();
            }
        }

        auto MakeSendEvent = [&]{
            // make event data
            event->frameNumber = scene->frameNumber;
            WriteTo(eventData, event);

            // send event
            for (auto &kv : ps) {
                kv.second->Send(eventData);
            }
        };

        if (!event->enters.empty()) {
            // temp clear new enter's scene ref, avoid send huge scene data
            for (auto &s : event->enters) {
                s->scene.Reset();
            }

            MakeSendEvent();

            // restore scene ref
            for (auto &s : event->enters) {
                s->scene = scene;
            }
        }
        else if (!event->css.empty() || !event->quits.empty()) {
            MakeSendEvent();
        }

        // logic update
        if (int r = scene->Update()) {
            // todo: game over?
            break;
        }

        // clear cache
        syncData.Clear();
        eventData.Clear();
        newEnters.clear();
        event->quits.clear();
        event->enters.clear();
        event->css.clear();
    }
    return 0;
}

int Server::HandlePackage(Peer &p, xx::ObjBase_s &o) {
    switch (o.typeId()) {
        case xx::TypeId_v<SS_C2S::Enter>: {
            assert(ps.contains(p.clientId));
            assert(!scene->shooters.contains(p.clientId));
            auto &&m = o.ReinterpretCast<SS_C2S::Enter>();
            auto &&r = newEnters.try_emplace(p.clientId, std::move(m));
            assert(r.second);
            enterResult->clientId = p.clientId;
            p.Send(enterResult);
            p.SetTimeoutSeconds(150);
//            LOG_INFO("clientId = ", p.clientId, " recv Enter: ", om.ToString(o));
            return 0;
        }
        case xx::TypeId_v<SS_C2S::Cmd>: {
            p.recvs.push(std::move(o));
            if (p.recvs.size() > 30) return -2;
            p.SetTimeoutSeconds(150);
//            LOG_INFO("clientId = ", p.clientId, " recv Cmd: ", om.ToString(m));
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


int Server::Accept(xx::Shared<Peer> const& p) {
    // 填充自增id
    p->clientId = ++pid;

    // 放入容器
    ps[p->clientId] = p;

    // 设置自动断线超时时间
    p->SetTimeoutSeconds(150);

    LOG_INFO("Accept success. ip = ", p->addr, ", clientId = ", p->clientId);

    // return success
    return 0;
}

void Server::Kick(const uint32_t &clientId) {
    // remove from container
    ps.erase(clientId);

    // remove shooter
    scene->shooters.erase(clientId);

    // add to event
    event->quits.push_back(clientId);
}
