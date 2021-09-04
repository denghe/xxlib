#include "AppDelegate.h"
#include "MainScene.h"

// 计算直线的弧度
template<typename Point1, typename Point2>
float GetAngle(Point1 const& from, Point2 const& to) noexcept {
	if (from.x == to.x && from.y == to.y) return 0.0f;
	auto&& len_y = to.y - from.y;
	auto&& len_x = to.x - from.x;
	return atan2f(len_y, len_x);
}

// 计算距离
template<typename Point1, typename Point2>
float GetDistance(Point1 const& a, Point2 const& b) noexcept {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx * dx + dy * dy);
}

// 点围绕 0,0 为中心旋转 a 弧度   ( 角度 * (float(M_PI) / 180.0f) )
template<typename Point>
inline Point Rotate(Point const& pos, float const& a) noexcept {
	auto&& sinA = sinf(a);
	auto&& cosA = cosf(a);
	return Point{ pos.x * cosA - pos.y * sinA, pos.x * sinA + pos.y * cosA };
}


bool MainScene::init() {
	if (!Scene::init()) return false;

	// calc size
	float dw = AppDelegate::designWidth, dh = AppDelegate::designHeight;
	float dw_2 = dw / 2, dh_2 = dh / 2;

	// create shooter instance
	shooter = std::make_shared<Shooter>(this, cocos2d::Point{ dw_2, dh_2 });

	// enable every frame call update
	scheduleUpdate();

	// move control
	keys.fill(false);
	auto listener = cocos2d::EventListenerKeyboard::create();
	listener->onKeyPressed = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
		keys[(int)keyCode] = true;
	};
	listener->onKeyReleased = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
		keys[(int)keyCode] = false;
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

	// mouse shoot
	auto listener2 = cocos2d::EventListenerTouchOneByOne::create();
	listener2->onTouchBegan = [this](cocos2d::Touch* t, cocos2d::Event*)->bool {
		touchPos = t->getLocation();
		return true;
	};
	listener2->onTouchMoved = [this](cocos2d::Touch* t, cocos2d::Event*) {
		if (touchPos.has_value()) {
			touchPos = t->getLocation();
		}
	};
	listener2->onTouchEnded = listener2->onTouchCancelled = [this](cocos2d::Touch*, cocos2d::Event*) {
		touchPos.reset();
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(listener2, this);

	// success
	return true;
}

void MainScene::update(float delta) {
	// handle input
	shooter->moveLeft = keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW] || keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_A];
	shooter->moveRight = keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW] || keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_D];
	shooter->moveUp = keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW] || keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_W];
	shooter->moveDown = keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW] || keys[(int)cocos2d::EventKeyboard::KeyCode::KEY_S];
	shooter->touchPos = touchPos;

	// keep 40 fps call update
	totalDelta += delta;
	while (totalDelta > 0.025f) {
		totalDelta -= 0.025f;
		if (shooter->Update()) {
			shooter.reset();
			// todo: show game over
			break;
		}
	}
}

Shooter::Shooter(MainScene* mainScene, cocos2d::Point pos)
	: mainScene(mainScene), pos(pos) {
	// draw
	body = cocos2d::Sprite::create("c.png");
	assert(body);
	body->setPosition(pos);
	mainScene->addChild(body);
}
Shooter::~Shooter() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
}

int Shooter::Update() {
	if (moveLeft) {
		pos.x -= 2;
	}
	if (moveRight) {
		pos.x += 2;
	}
	if (moveUp) {
		pos.y += 2;
	}
	if (moveDown) {
		pos.y -= 2;
	}
	body->setPosition(pos);

	for (int i = (int)bullets.size() - 1; i >= 0; --i) {
		auto& b = bullets[i];
		if (b->Update()) {
			if (auto n = (int)bullets.size() - 1; i < n) {
				b = std::move(bullets[n]);
			}
			bullets.pop_back();
		}
	}

	if (touchPos) {
		auto&& b = bullets.emplace_back();

		auto angle = GetAngle(pos, touchPos.value());
		auto inc1 = Rotate(cocos2d::Point{ 147, 0 }, angle);
		auto inc2 = Rotate(cocos2d::Point{ 30, 0 }, angle);
		b = std::make_shared<Bullet>(this, pos + inc1, inc2, 40);
	}

	return 0;
}

Bullet::Bullet(Shooter* shooter, cocos2d::Point pos, cocos2d::Point inc, int life)
	: shooter(shooter)
	, mainScene(shooter->mainScene)
	, pos(pos)
	, inc(inc)
	, life(life) {
	// draw
	body = cocos2d::Sprite::create("b.png");
	assert(body);
	body->setPosition(pos);
	mainScene->addChild(body);
}

int Bullet::Update() {
	if (life < 0) return 1;
	--life;
	pos += inc;
	body->setPosition(pos);
	return 0;
}

Bullet::~Bullet() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
}
