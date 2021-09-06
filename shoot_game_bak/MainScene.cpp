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

	// create container
	container = cocos2d::Node::create();
	container->setPosition(dw_2, dh_2);
	this->addChild(container);

	// create cursor
	cursor = cocos2d::Sprite::create("a.png");
	cursor->setPosition(dw_2, dh_2);
	cursor->setScale(3);
	this->addChild(cursor);

	// create shooter instance
	shooter = std::make_shared<Shooter>(this, cocos2d::Point{ 0,0 });

	// enable every frame call update
	scheduleUpdate();

	// init keyboard event listener
	{
		keyboards.fill(false);
		auto L = cocos2d::EventListenerKeyboard::create();
		L->onKeyPressed = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
			keyboards[(int)keyCode] = true;
		};
		L->onKeyReleased = [this](cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event) {
			keyboards[(int)keyCode] = false;
		};
		_eventDispatcher->addEventListenerWithSceneGraphPriority(L, container);
	}

	// init touch event listener
	//{
	//	auto L = cocos2d::EventListenerTouchOneByOne::create();
	//	L->onTouchBegan = [this](cocos2d::Touch* t, cocos2d::Event*)->bool {
	//		touchPos = container->convertToNodeSpace(t->getLocation());
	//		return true;
	//	};
	//	L->onTouchMoved = [this](cocos2d::Touch* t, cocos2d::Event*) {
	//		if (touchPos.has_value()) {
	//			touchPos = container->convertToNodeSpace(t->getLocation());
	//		}
	//	};
	//	L->onTouchEnded = L->onTouchCancelled = [this](cocos2d::Touch*, cocos2d::Event*) {
	//		touchPos.reset();
	//	};
	//	_eventDispatcher->addEventListenerWithSceneGraphPriority(L, container);
	//}

	// init mouse event listener
	{
		mouseKeys.fill(false);
		auto L = cocos2d::EventListenerMouse::create();
		L->onMouseMove = [this](cocos2d::EventMouse* e) {
			cocos2d::Point p(e->getCursorX(), e->getCursorY());
			cursor->setPosition(p);
			mousePos = container->convertToNodeSpace(p);
		};
		L->onMouseUp = [this](cocos2d::EventMouse* e) {
			mouseKeys[(int)e->getMouseButton() + 1] = false;
		};
		L->onMouseDown = [this](cocos2d::EventMouse* e) {
			mouseKeys[(int)e->getMouseButton() + 1] = true;
		};
		L->onMouseScroll = [this](cocos2d::EventMouse* e) {
			//e->getScrollY()
		};
		_eventDispatcher->addEventListenerWithSceneGraphPriority(L, container);
	}

	// hide mouse
	cocos2d::Director::getInstance()->getOpenGLView()->setCursorVisible(false);

	// success
	return true;
}

void MainScene::update(float delta) {
	// handle input
	if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_Z] && zoom > 0.02f) {
		zoom -= 0.005f;
	}
	if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_X] && zoom < 1.f) {
		zoom += 0.005f;
	}
	container->setScale(zoom);

	shooter->moveLeft = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_A];
	shooter->moveRight = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_D];
	shooter->moveUp = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_W];
	shooter->moveDown = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_S];

	shooter->button1 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_LEFT + 1];
	shooter->button2 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_RIGHT + 1];
	shooter->aimPos = mousePos;

	// keep 60 fps call update
	totalDelta += delta;
	while (totalDelta > (1.f / 60.f)) {
		totalDelta -= (1.f / 60.f);
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
	mainScene->container->addChild(body);

	gun = cocos2d::Sprite::create("c.png");
	assert(gun);
	gun->setScale(0.3f);
	gun->setPosition(pos + cocos2d::Point(147, 0));
	mainScene->container->addChild(gun);
}
Shooter::~Shooter() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
	if (gun) {
		gun->removeFromParent();
		gun = nullptr;
	}
}

int Shooter::Update() {
	// rotate
	bodyAngle += 1.f;
	if (bodyAngle > 360.f) {
		bodyAngle -= 360.f;
	}
	body->setRotation(bodyAngle);
	gun->setRotation(360.f - bodyAngle * 3.333f);

	// move shooter
	if (moveLeft) {
		pos.x -= moveDistancePerFrame;
	}
	if (moveRight) {
		pos.x += moveDistancePerFrame;
	}
	if (moveUp) {
		pos.y += moveDistancePerFrame;
	}
	if (moveDown) {
		pos.y -= moveDistancePerFrame;
	}
	body->setPosition(pos);

	// sync gun pos 
	auto angle = GetAngle(pos, aimPos);
	auto gunPosOffset = Rotate(cocos2d::Point{ 147, 0 }, angle);
	auto gunPos = pos + gunPosOffset;
	gun->setPosition(pos + gunPosOffset);

	// bullets
	for (int i = (int)bullets.size() - 1; i >= 0; --i) {
		auto& b = bullets[i];
		if (b->Update()) {
			if (auto n = (int)bullets.size() - 1; i < n) {
				b = std::move(bullets[n]);
			}
			bullets.pop_back();
		}
	}

	// emit bullets
	if (button1) {
		auto&& b = bullets.emplace_back();
		auto inc = Rotate(cocos2d::Point{ 30, 0 }, angle);
		b = std::make_shared<Bullet>(this, gunPos, inc, 40);
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
	mainScene->container->addChild(body);
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
