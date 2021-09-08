#include "AppDelegate.h"
#include "MainScene.h"
#include <iostream>
#include <xx_math.h>

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

	// create ui
	ui = cocos2d::Node::create();
	ui->setPosition(dw_2, dh_2);
	ui->setScale(zoom);
	this->addChild(ui);

	// create cursor
	cursor = cocos2d::Sprite::create("a.png");
	cursor->setPosition(dw_2, 0);
	cursor->setScale(3);
	this->addChild(cursor);

	// create cache packages
	cmd.Emplace();
	// ...

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

	// ��ִ������Э��
	lineNumber = Update();
	assert(lineNumber);

	// ���û�н��뵽 �� ״̬ ��ֱ�Ӷ�·�˳�
	if (!playing) return;

	// ���Ŵ���( ������Ϊ )
	if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_Z] && zoom > 0.05f) {
		zoom -= 0.005f;
	}
	if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_X] && zoom < 1.f) {
		zoom += 0.005f;
	}
	container->setScale(zoom);

	// ���ɵ�ǰ����״̬
	SS::ControlState newCS;
	newCS.aimPos = { (int)mousePos.x, (int)mousePos.y };
	newCS.moveLeft = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_A];
	newCS.moveRight = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_D];
	newCS.moveUp = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_W];
	newCS.moveDown = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_S];
	newCS.button1 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_LEFT + 1];
	newCS.button2 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_RIGHT + 1];

	// ����ͱ��ݲ�һ�£��ͷ����� server
	if (cmd->cs != newCS) {
		cmd->cs = newCS;
		c.Send(cmd);
	}
}

int MainScene::Update() {
	c.Update();

	COR_BEGIN;

	// ��ʼ�����ŵ�ַ
	c.SetDomainPort("192.168.1.95", 12345);

LabBegin:

	DrawInit();
	// ��������һ��
	c.Reset();
	cmd.Emplace();
	playing = false;

	// ˯һ��
	secs = xx::NowEpochSeconds() + 1;
	do {
		COR_YIELD;
	} while (secs > xx::NowEpochSeconds());


	DrawResolve();
	// ��ʼ��������
	c.Resolve();

	// �� 3 ��, ���������ɾ�ֹͣ�ȴ�
	secs = xx::NowEpochSeconds() + 3;
	do {
		COR_YIELD;
		if (!c.Busy()) break;
	} while (secs > xx::NowEpochSeconds());

	// ����ʧ��: ����
	if (!c.IsResolved()) goto LabBegin;

	//// ��ӡ�� ip �б�
	//for (auto& ip : c.GetIPList()) {
	//	xx::CoutTN(ip);
	//}

	DrawDial();
	// ��ʼ����
	if (int r = c.Dial()) {
		xx::CoutTN("c.Dial() = ", r);
		goto LabBegin;
	}

	// �� 3 ��, ���������ɾ�ֹͣ�ȴ�
	secs = xx::NowEpochSeconds() + 3;
	do {
		COR_YIELD;
		if (!c.Busy()) break;
	} while (secs > xx::NowEpochSeconds());

	// û����: ����
	if (!c.Alive()) goto LabBegin;


	// ��һ��Ĭ��ֵ cmd ��Ϊ�������󡣷����� ���� Sync��
	synced = false;
	c.Send(cmd);

	// �� 3 ��, ���û���յ� sync ����������
	secs = xx::NowEpochSeconds() + 3;
	do {
		COR_YIELD;
		
		// ���Դ��հ����л�ȡ��
		xx::ObjBase_s o;
		if (c.TryGetPackage(o)) {
			// ���Ϊ �� �� ���� sync �� ����������
			if (!o || o.typeId() != xx::TypeId_v<SS_S2C::Sync>) goto LabBegin;
			auto&& sync = o.ReinterpretCast<SS_S2C::Sync>();
			scene = std::move(sync->scene);
			synced = true;
		}

	} while (secs > xx::NowEpochSeconds());
	if (!synced) goto LabBegin;


	DrawPlay();
	playing = true;
	// ��ʼ��Ϸ��������߾�����
	do {
		COR_YIELD;

		// �������� events ֱ��û��
		xx::ObjBase_s o;
		// ���Դ��հ����л�ȡ��
		if (c.TryGetPackage(o)) {
			// ����Ӧ�þ��� Event
			assert(o && o.typeId() == xx::TypeId_v<SS_S2C::Event>);
			auto&& e = o.ReinterpretCast<SS_S2C::Event>();
			assert(e->frameNumber > scene->frameNumber);

			// ׷֡
			do {
				// todo: ���� e->cs �� Update
				scene->Update();
			} while (e->frameNumber == scene->frameNumber);
		}
		else {
			scene->Update();
		}

	} while (c.Alive());
	goto LabBegin;

	COR_END;
}

void MainScene::DrawInit() {
	ui->removeAllChildrenWithCleanup(true);
	auto lbl = cocos2d::Label::createWithSystemFont("init", "", 64);
	ui->addChild(lbl);
}
void MainScene::DrawResolve() {
	ui->removeAllChildrenWithCleanup(true);
	auto lbl = cocos2d::Label::createWithSystemFont("resolve domain...", "", 64);
	ui->addChild(lbl);
}
void MainScene::DrawDial() {
	ui->removeAllChildrenWithCleanup(true);
	auto lbl = cocos2d::Label::createWithSystemFont("dial...", "", 64);
	ui->addChild(lbl);
}
void MainScene::DrawPlay() {
	ui->removeAllChildrenWithCleanup(true);
	// todo: draw scene
}

//Shooter::Shooter(MainScene* mainScene, XY pos)
//	: mainScene(mainScene), pos(pos) {
//	// draw
//	body = cocos2d::Sprite::create("c.png");
//	assert(body);
//	body->setPosition(pos);
//	mainScene->container->addChild(body);
//
//	gun = cocos2d::Sprite::create("c.png");
//	assert(gun);
//	gun->setScale(0.3f);
//	gun->setPosition((float)(pos.x + 147), (float)pos.y);
//	mainScene->container->addChild(gun);
//}
//Shooter::~Shooter() {
//	if (body) {
//		body->removeFromParent();
//		body = nullptr;
//	}
//	if (gun) {
//		gun->removeFromParent();
//		gun = nullptr;
//	}
//}
//
//int Shooter::Update() {
//	// rotate
//	bodyAngle += 1.f;
//	if (bodyAngle > 360.f) {
//		bodyAngle -= 360.f;
//	}
//	body->setRotation(bodyAngle);
//	gun->setRotation(360.f - bodyAngle * 3.333f);
//
//	// move shooter
//	if (moveLeft) {
//		pos.x -= moveDistancePerFrame;
//	}
//	if (moveRight) {
//		pos.x += moveDistancePerFrame;
//	}
//	if (moveUp) {
//		pos.y += moveDistancePerFrame;
//	}
//	if (moveDown) {
//		pos.y -= moveDistancePerFrame;
//	}
//	body->setPosition(pos);
//
//	// sync gun pos 
//	auto angle = GetAngle(pos, aimPos);
//	auto gunPosOffset = Rotate(XY{ 147, 0 }, angle);
//	auto gunPos = XY{ pos.x + gunPosOffset.x, pos.y + gunPosOffset.y };
//	std::cout << (uint32_t)angle << ", " << gunPosOffset.x << ", " << gunPosOffset.y << std::endl;
//	gun->setPosition(gunPos);
//
//	// bullets
//	for (int i = (int)bullets.size() - 1; i >= 0; --i) {
//		auto& b = bullets[i];
//		if (b->Update()) {
//			if (auto n = (int)bullets.size() - 1; i < n) {
//				b = std::move(bullets[n]);
//			}
//			bullets.pop_back();
//		}
//	}
//
//	// emit bullets
//	if (button1) {
//		auto&& b = bullets.emplace_back();
//		auto inc = Rotate(XY{ 30, 0 }, angle);
//		b = std::make_shared<Bullet>(this, gunPos, inc, 1000);
//	}
//
//	return 0;
//}
//
//Bullet::Bullet(Shooter* shooter, XY pos, XY inc, int life)
//	: shooter(shooter)
//	, mainScene(shooter->mainScene)
//	, pos(pos)
//	, inc(inc)
//	, life(life) {
//	// draw
//	body = cocos2d::Sprite::create("b.png");
//	assert(body);
//	body->setPosition(pos);
//	mainScene->container->addChild(body);
//}
//
//int Bullet::Update() {
//	if (life < 0) return 1;
//	--life;
//	pos.x += inc.x;
//	pos.y += inc.y;
//	body->setPosition(pos);
//	return 0;
//}
//
//Bullet::~Bullet() {
//	if (body) {
//		body->removeFromParent();
//		body = nullptr;
//	}
//}



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



			// todo: Ĭ������£����ȶ�֡�ʼ������㡣
			//// keep 60 fps call update
	//totalDelta += delta;
	//while (totalDelta > (1.f / 60.f)) {
	//	totalDelta -= (1.f / 60.f);
	//	if (shooter->Update()) {
	//		shooter.reset();
	//		// todo: show game over
	//		break;
	//	}
	//}




		//{
		//    auto o = xx::Make<SS::Bullet>();
		//    c.Send(o);
		//}
		//if (auto siz = c.receivedPackages.size()) {
		//	xx::CoutN(siz);
		//}
