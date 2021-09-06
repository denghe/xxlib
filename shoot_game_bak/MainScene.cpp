#include "AppDelegate.h"
#include "MainScene.h"
#include <iostream>

/***********************************************************************************************/
// 整数坐标系游戏常用函数/查表库。主要意图为确保跨硬件计算结果的一致性( 附带性能有一定提升 )

static const double pi2 = 3.14159265358979323846264338328 * 2;

// 设定计算坐标系 x, y 值范围 为 正负 table_xy_range
static const int table_xy_range = 2048, table_xy_range2 = table_xy_range * 2, table_xy_rangePOW2 = table_xy_range * table_xy_range;

// 角度分割精度。256 对应 uint8_t，65536 对应 uint16_t, ... 对于整数坐标系来说，角度继续细分意义不大。增量体现不出来
static const int table_num_angles = 65536;
using table_angle_element_type = uint16_t;

// 查表 sin cos 整数值 放大系数
static const int64_t table_sincos_ratio = 10000;

// 构造一个查表数组，下标是 +- table_xy_range 二维坐标值。内容是以 table_num_angles 为切割单位的自定义角度值( 并非 360 或 弧度 )
inline std::array<table_angle_element_type, table_xy_range2* table_xy_range2> table_angle;

// 基于 table_num_angles 个角度，查表 sin cos 值 ( 通常作为移动增量 )
inline std::array<int, table_num_angles> table_sin;
inline std::array<int, table_num_angles> table_cos;


// 程序启动时自动填表
struct TableFiller {
	TableFiller() {
		// todo: 多线程计算提速
		for (int y = -table_xy_range; y < table_xy_range; ++y) {
			for (int x = -table_xy_range; x < table_xy_range; ++x) {
				auto idx = (y + table_xy_range) * table_xy_range2 + (x + table_xy_range);
				auto a = atan2((double)y, (double)x);
				if (a < 0) a += pi2;
				table_angle[idx] = (table_angle_element_type)(a / pi2 * table_num_angles);
			}
		}
		// fix same x,y
		table_angle[(0 + table_xy_range) * table_xy_range2 + (0 + table_xy_range)] = 0;

		for (int i = 0; i < table_num_angles; ++i) {
			auto s = sin((double)i / table_num_angles * pi2);
			table_sin[i] = (int)(s * table_sincos_ratio);
			auto c = cos((double)i / table_num_angles * pi2);
			table_cos[i] = (int)(c * table_sincos_ratio);
		}
	}
};
inline TableFiller tableFiller__;

// 传入坐标，返回角度值( 0 ~ table_num_angles )  safeMode: 支持大于查表尺寸的 xy 值 ( 慢几倍 )
template<bool safeMode = true>
table_angle_element_type GetAngleXY(int x, int y) noexcept {
	if constexpr (safeMode) {
		while (x < -table_xy_range || x >= table_xy_range || y < -table_xy_range || y >= table_xy_range) {
			x /= 8;
			y /= 8;
		}
	}
	else {
		assert(x >= -table_xy_range && x < table_xy_range&& y >= -table_xy_range && y < table_xy_range);
	}
	return table_angle[(y + table_xy_range) * table_xy_range2 + x + table_xy_range];
}
template<bool safeMode = true>
table_angle_element_type GetAngleXYXY(int const& x1, int const& y1, int const& x2, int const& y2) noexcept {
	return GetAngleXY<safeMode>(x2 - x1, y2 - y1);
}
template<bool safeMode = true, typename Point1, typename Point2>
table_angle_element_type GetAngle(Point1 const& from, Point2 const& to) noexcept {
	return GetAngleXY<safeMode>(to.x - from.x, to.y - from.y);
}

// 计算点旋转后的坐标
inline XY Rotate(XY const& p, table_angle_element_type const& a) noexcept {
	auto s = (int64_t)table_sin[a];
	auto c = (int64_t)table_cos[a];
	//return { (int)(p.x * c / table_sincos_ratio - p.y * s / table_sincos_ratio), (int)(p.x * s / table_sincos_ratio + p.y * c / table_sincos_ratio) };
	return { (int)((p.x * c - p.y * s) / table_sincos_ratio), (int)((p.x * s + p.y * c) / table_sincos_ratio) };
}

/***********************************************************************************************/



bool MainScene::init() {
	if (!Scene::init()) return false;

	// calc size
	float dw = AppDelegate::designWidth, dh = AppDelegate::designHeight;
	float dw_2 = dw / 2, dh_2 = dh / 2;

	// create container
	container = cocos2d::Node::create();
	container->setPosition(dw_2, dh_2);
	container->setScale(zoom);
	this->addChild(container);

	// create cursor
	cursor = cocos2d::Sprite::create("a.png");
	cursor->setPosition(dw_2, dh_2);
	cursor->setScale(3);
	this->addChild(cursor);

	// create shooter instance
	shooter = std::make_shared<Shooter>(this, XY{ 0,0 });

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
	if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_Z] && zoom > 0.05f) {
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
	shooter->aimPos = XY{ (int)mousePos.x, (int)mousePos.y };

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

Shooter::Shooter(MainScene* mainScene, XY pos)
	: mainScene(mainScene), pos(pos) {
	// draw
	body = cocos2d::Sprite::create("c.png");
	assert(body);
	body->setPosition(pos);
	mainScene->container->addChild(body);

	gun = cocos2d::Sprite::create("c.png");
	assert(gun);
	gun->setScale(0.3f);
	gun->setPosition((float)(pos.x + 147), (float)pos.y);
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
	auto gunPosOffset = Rotate(XY{ 147, 0 }, angle);
	auto gunPos = XY{ pos.x + gunPosOffset.x, pos.y + gunPosOffset.y };
	std::cout << (uint32_t)angle << ", " << gunPosOffset.x << ", " << gunPosOffset.y << std::endl;
	gun->setPosition(gunPos);

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
		auto inc = Rotate(XY{ 30, 0 }, angle);
		b = std::make_shared<Bullet>(this, gunPos, inc, 1000);
	}

	return 0;
}

Bullet::Bullet(Shooter* shooter, XY pos, XY inc, int life)
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
	pos.x += inc.x;
	pos.y += inc.y;
	body->setPosition(pos);
	return 0;
}

Bullet::~Bullet() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
}
