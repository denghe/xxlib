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

	// store singleton
	instance = this;

	// success
	return true;
}

void MainScene::update(float delta) {
	// 网络收发
	c.Update();

	// frame limit
	totalDelta += delta;
	while (totalDelta > (1.f / 60.f)) {
		totalDelta -= (1.f / 60.f);

		// 先执行主线协程
		lineNumber = Update();
		assert(lineNumber);

		// 如果没有进入到 玩 状态 就直接短路退出
		if (!playing) return;

		// 缩放处理( 本地行为 )
		if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_Z] && zoom > 0.05f) {
			zoom -= 0.005f;
		}
		if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_X] && zoom < 1.f) {
			zoom += 0.005f;
		}
		container->setScale(zoom);

		// 生成当前控制状态
		SS::ControlState newCS;
		newCS.aimPos = { (int)mousePos.x, (int)mousePos.y };
		newCS.moveLeft = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_A];
		newCS.moveRight = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_D];
		newCS.moveUp = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_W];
		newCS.moveDown = keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW] || keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_S];
		newCS.button1 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_LEFT + 1];
		newCS.button2 = mouseKeys[(int)cocos2d::EventMouse::MouseButton::BUTTON_RIGHT + 1];

		// 如果和备份不一致，就发包到 server
		if (cmd->cs != newCS) {
			cmd->cs = newCS;
			c.Send(cmd);
		}

	}

	// 网络收发( 再来一次. 否则可能收不完 )
	c.Update();
}

int MainScene::Update() {
	COR_BEGIN;

	// 初始化拨号地址
	c.SetDomainPort("192.168.1.95", 12345);

LabBegin:

	DrawInit();
	// 无脑重置一发
	c.Reset();
	cmd.Emplace();
	playing = false;

	// 睡一秒
	secs = xx::NowEpochSeconds() + 1;
	do {
		COR_YIELD;
	} while (secs > xx::NowEpochSeconds());


	DrawResolve();
	// 开始域名解析
	c.Resolve();

	// 等 3 秒, 如果解析完成就停止等待
	secs = xx::NowEpochSeconds() + 3;
	do {
		COR_YIELD;
		if (!c.Busy()) break;
	} while (secs > xx::NowEpochSeconds());

	// 解析失败: 重来
	if (!c.IsResolved()) goto LabBegin;

	//// 打印下 ip 列表
	//for (auto& ip : c.GetIPList()) {
	//	xx::CoutTN(ip);
	//}

	DrawDial();
	// 开始拨号
	if (int r = c.Dial()) {
		xx::CoutTN("c.Dial() = ", r);
		goto LabBegin;
	}

	// 等 10 秒, 如果拨号完成就停止等待
	secs = xx::NowEpochSeconds() + 10;
	do {
		COR_YIELD;
		if (!c.Busy()) break;
	} while (secs > xx::NowEpochSeconds());

	// 没连上: 重来
	if (!c.Alive()) goto LabBegin;


	// 发 进入 请求。服务器 返回 Sync。
	synced = false;
	c.Send(xx::Make<SS_C2S::Enter>());

	// 等 5 秒, 如果没有收到 sync 就掐线重连
	secs = xx::NowEpochSeconds() + 5;
	do {
		COR_YIELD;

		// 尝试从收包队列获取包
		xx::ObjBase_s o;
		if (c.TryGetPackage(o)) {
			// 如果为 空 或 不是 sync 包 就掐线重连
			if (!o || o.typeId() != xx::TypeId_v<SS_S2C::Sync>) goto LabBegin;
			auto&& sync = o.ReinterpretCast<SS_S2C::Sync>();
			scene = std::move(sync->scene);
			synced = true;
			break;
		}

	} while (secs > xx::NowEpochSeconds());
	if (!synced) goto LabBegin;


	DrawPlay();
	playing = true;
	// 开始游戏。如果断线就重连
	do {
		COR_YIELD;

		if (!c.receivedPackages.empty()) {
			// 继续处理 events 直到没有
			xx::ObjBase_s o;
			// 尝试从收包队列获取包
			while (c.TryGetPackage(o)) {
				// 类型应该就是 Event
				assert(o && o.typeId() == xx::TypeId_v<SS_S2C::Event>);
				auto&& e = o.ReinterpretCast<SS_S2C::Event>();

				if (e->frameNumber > scene->frameNumber) {
					xx::CoutTN("fast forward from ", scene->frameNumber, " to ", e->frameNumber);
					do {
						// 追帧并备份
						scene->Update();
						Backup();
					} while (e->frameNumber == scene->frameNumber);
				} else if (e->frameNumber < scene->frameNumber) {
					// 回滚。失败则重新拨号
					if (int r = Rollback(e->frameNumber)) goto LabBegin;
				}
				// 应用 event
				scene->shooter->cs = e->cs;
				// 更新场景并备份
				scene->Update();
				Backup();
			}
		} else {
			// 更新场景并备份
			scene->Update();
			Backup();
		}
		scene->Draw();

	} while (c.Alive());
	goto LabBegin;

	COR_END;
}

void MainScene::Backup() {
	// 首次备份: 记录帧编号
	if (frameBackups.Empty()) {
		frameBackupsFirstFrameNumber = scene->frameNumber;
	} else {
		// 如果队列超出限定长度就 pop 一个并同步 [0] 的 frameNumber
		if (frameBackups.Count() == maxBackupCount) {
			frameBackups.Pop();
			++frameBackupsFirstFrameNumber;
		}
	}
	// 备份
	auto& d = frameBackups.Emplace();
	c.om.WriteTo(d, scene);

	// 核查
	assert(scene->frameNumber + 1 == frameBackupsFirstFrameNumber + frameBackups.Count());
}

int MainScene::Rollback(int const& frameNumber) {
	xx::CoutTN("rollback from ", scene->frameNumber, " to ", frameNumber);

	// 核查
	assert(frameNumber < frameBackupsFirstFrameNumber + frameBackups.Count());

	// 如果超出范围就返回失败( 过期了 )
	if (frameNumber < frameBackupsFirstFrameNumber) return -1;

	// 计算出目标帧的下标
	auto idx = frameNumber - frameBackupsFirstFrameNumber;

	// 将数据挪出来
	auto d = std::move(frameBackups[idx]);

	// 回滚( 相当于收到 sync )
	c.om.ReadFrom(d, scene);

	// 清空队列，将 d 放入，同步 [0] 的 frameNumber
	frameBackups.Clear();
	frameBackups.Push(std::move(d));
	frameBackupsFirstFrameNumber = frameNumber;

	return 0;
}

void MainScene::DrawInit() {
	ui->removeAllChildrenWithCleanup(true);
	auto lbl = cocos2d::Label::createWithSystemFont("init", "", 64);
	ui->addChild(lbl);
	scene.Reset();
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
	scene->Draw();
}



void SS::Scene::Draw() {
	shooter->Draw();
}


void SS::Shooter::Draw() {

	if (!body) {
		body = cocos2d::Sprite::create("c.png");
		assert(body);
		MainScene::instance->container->addChild(body);
	}
	body->setRotation((float)bodyAngle);
	body->setPosition(pos);

	if (!gun) {
		gun = cocos2d::Sprite::create("c.png");
		assert(gun);
		gun->setScale(0.3f);
		MainScene::instance->container->addChild(gun);
	}
	auto angle = xx::GetAngle(pos, cs.aimPos);
	auto gunPosOffset = xx::Rotate(XY{ 147, 0 }, angle);
	auto gunPos = XY{ pos.x + gunPosOffset.x, pos.y + gunPosOffset.y };
	gun->setPosition(gunPos);
	gun->setRotation(360.f - bodyAngle * 3.333f);

	for (auto& b : bullets) {
		b->Draw();
	}
}
SS::Shooter::~Shooter() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
	if (gun) {
		gun->removeFromParent();
		gun = nullptr;
	}
}


void SS::Bullet::Draw() {
	if (!body) {
		body = cocos2d::Sprite::create("b.png");
		assert(body);
		MainScene::instance->container->addChild(body);
	}
	body->setPosition(pos);
}
SS::Bullet::~Bullet() {
	if (body) {
		body->removeFromParent();
		body = nullptr;
	}
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
