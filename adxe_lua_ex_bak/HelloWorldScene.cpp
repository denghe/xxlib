#include "HelloWorldScene.h"
#include <xx_helpers.h>

static inline float originX, originY, winWidth, winHeight, centerX, centerY;

HelloWorld::HelloWorld() : coros(60, 60 * 60 * 5) {}

bool HelloWorld::init() {
    if (!Scene::init()) return false;

    auto&& safeArea = cocos2d::Director::getInstance()->getSafeAreaRect();
    originX = safeArea.origin.x;
    originY = safeArea.origin.y;
    winWidth = safeArea.size.width;
    winHeight = safeArea.size.height;
    centerX = (originX + winWidth) / 2;
    centerY = (originY + winHeight) / 2;

	// enable update(float delta)
    scheduleUpdate();

	// every 2 seconds print coros.
	coros.Add([](HelloWorld* scene)->xx::Coro {
		while (true) {
			cocos2d::log("coros.count = %d", scene->coros.nodes.Count());
			co_yield 2;
		}
	}(this));

    return true;
}

void HelloWorld::update(float delta) {
	// every frame create 10 sprites
	for (size_t i = 0; i < 10; i++) {
		coros.Add([](HelloWorld* scene)->xx::Coro {
			co_yield 0;
			// start pos : center screen
			auto x = centerX;
			auto y = centerY;
			// random angle
			auto a = (float)rand() / RAND_MAX * 3.1416f * 2.f;
			// set speed
			float spd = 1.f;
			// calculate frame increase value
			auto dx = cosf(a) * spd;
			auto dy = sinf(a) * spd;
			// calculate end time
			auto et = xx::NowEpochSeconds() + 100;
			// create sprite
			auto s = cocos2d::Sprite::create("HelloWorld.png");
			s->setPosition(x, y);
			scene->addChild(s);
			do {
				x += dx;
				y += dy;
				s->setPosition(x, y);
				co_yield 0;
			} while (et > xx::NowEpochSeconds());
			// kill sprite
			s->removeFromParent();
		}(this));
	}
	// call all coroutines
    coros();
}
