#include "ss.h"
#include "xx_math.h"

int SS::Scene::Update() {
    ++frameNumber;

    // todo: update 过程中移除
    for (auto &kv : shooters) {
        if (int r = kv.second->Update()) {
            // ...
        }
    }

    return shooters.empty() ? 1 : 0;
}

int SS::Shooter::Update() {
    // 身体自转( 只是营造显示效果，不影响业务逻辑 )
    bodyAngle += 1;
    if (bodyAngle >= 360) {
        bodyAngle = 0;
    }

    // 移动发射者
    if (cs.moveLeft) {
        pos.x -= speed;
    }
    if (cs.moveRight) {
        pos.x += speed;
    }
    if (cs.moveUp) {
        pos.y += speed;
    }
    if (cs.moveDown) {
        pos.y -= speed;
    }

    // todo: 移动范围限定

    // 同步枪的坐标
    auto angle = xx::GetAngle(pos, cs.aimPos);
    auto gunPosOffset = xx::Rotate(SS::XY{147, 0}, angle);
    auto gunPos = SS::XY{pos.x + gunPosOffset.x, pos.y + gunPosOffset.y};

    // 推进子弹逻辑
    for (int i = (int) bullets.size() - 1; i >= 0; --i) {
        auto &b = bullets[i];
        if (b->Update()) {
            if (auto n = (int) bullets.size() - 1; i < n) {
                b = std::move(bullets[n]);
            }
            bullets.pop_back();
        }
    }

    // 发射直线子弹
    if (cs.button1) {
        auto &&b = xx::Make<SS::Bullet_Straight>();
        b->shooter = WeakFromThis<Shooter>();
        b->life = 1000;
        b->pos = gunPos;
        b->inc = xx::Rotate(SS::XY{30, 0}, angle);
        bullets.emplace_back(std::move(b));
    }

    // 发射跟踪子弹
    if (cs.button2) {
        // 查找距离 鼠标 一段距离内的 其他 shooter
        xx::Weak<Shooter> tar;
        assert(scene);
        for (auto &kv : scene->shooters) {
            // skip self
            if (kv.first == clientId) continue;
            // pickup check: distance < r1 + r2
            if (xx::DistanceNear(50, 100, cs.aimPos, kv.second->pos)) {     // todo: 给 bullet 和 shooter 加上 半径 参数. 鼠标拾取半径为 常量
                tar = kv.second;
            }
        }
        if (tar) {
            auto &&b = xx::Make<SS::Bullet_Track>();
            b->shooter = WeakFromThis<Shooter>();
            b->life = 1000;
            b->pos = gunPos;
            b->target = tar;
            bullets.emplace_back(std::move(b));
        }
    }

    // success
    return 0;
}

int SS::Bullet_Straight::Update() {
    if (life < 0) return 1;
    --life;
    // todo: collision detect. b2b: all die. b2s: b die. s change color a while.

    pos += inc;
    return 0;
}

int SS::Bullet_Track::Update() {
    if (life < 0) return 1;
    --life;
    auto tar = target.Lock();
    if (!tar) return 2;

    // todo: collision detect. b2b: all die. b2s: b die. s change color a while.

    // aim target, get inc by speed
    auto a = xx::GetAngle(pos, tar->pos);
    auto inc = xx::Rotate(SS::XY{speed, 0}, a);
    pos += inc;
    return 0;
}
