#include "ss.h"
#include "xx_math.h"

int SS::Scene::Update() {
    ++frameNumber;

    //return shooter->Update();
    return 0;
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

    // 同步枪的坐标
    auto angle = xx::GetAngle(pos, cs.aimPos);
    auto gunPosOffset = xx::Rotate({147, 0}, angle);
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
        auto&& b = xx::Make<SS::Bullet_Straight>();
        b->shooter = WeakFromThis<Shooter>();
        b->life = 1000;
        b->pos = gunPos;
        b->inc = xx::Rotate({30, 0}, angle);
        bullets.push_back(std::move(b));
    }

    // 发射跟踪子弹
    if (cs.button2) {
        auto&& b = xx::Make<SS::Bullet_Track>();
        b->shooter = WeakFromThis<Shooter>();
        b->life = 1000;
        b->pos = gunPos;
        //b->target = ??????????  // todo: 查找距离 鼠标 一段距离内的 其他 shooter
        bullets.push_back(std::move(b));
    }

    // success
    return 0;
}

int SS::Bullet_Straight::Update() {
    if (life < 0) return 1;
    --life;
    pos.x += inc.x;
    pos.y += inc.y;
    return 0;
}

int SS::Bullet_Track::Update() {
    if (life < 0) return 1;
    --life;
    // todo
    return 0;
}
