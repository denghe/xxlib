#include "ss.h"
#include "xx_math.h"
// todo: #if is client draw xxxxxxxxxx

int SS::Scene::Update() {
    return shooter->Update();
}

int SS::Shooter::Update() {
    // rotate
    bodyAngle += 1.f;
    if (bodyAngle > 360.f) {
        bodyAngle = 1.f;
    }

    // move shooter
    if (cs.moveLeft) {
        pos.x -= moveDistancePerFrame;
    }
    if (cs.moveRight) {
        pos.x += moveDistancePerFrame;
    }
    if (cs.moveUp) {
        pos.y += moveDistancePerFrame;
    }
    if (cs.moveDown) {
        pos.y -= moveDistancePerFrame;
    }

    // sync gun pos
    auto angle = xx::GetAngle(pos, cs.aimPos);
    auto gunPosOffset = xx::Rotate({147, 0}, angle);
    auto gunPos = SS::XY{pos.x + gunPosOffset.x, pos.y + gunPosOffset.y};

    // call bullets logic
    for (int i = (int) bullets.size() - 1; i >= 0; --i) {
        auto &b = bullets[i];
        if (b->Update()) {
            if (auto n = (int) bullets.size() - 1; i < n) {
                b = std::move(bullets[n]);
            }
            bullets.pop_back();
        }
    }

    // emit bullets
    if (cs.button1) {
        auto &&b = *bullets.emplace_back().Emplace();
        b.inc = xx::Rotate({30, 0}, angle);
        b.life = 1000;
        b.pos = gunPos;
        //b.owner = this;
    }

    // success
    return 0;
}

int SS::Bullet::Update() {
    if (life < 0) return 1;
    --life;
    pos.x += inc.x;
    pos.y += inc.y;
    return 0;
}
