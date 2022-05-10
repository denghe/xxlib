#include "AppDelegate.h"
#include "HelloWorldScene.h"
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

	// begin coroutine for logic
	co_spawn(c.ioc, [this]()->awaitable<void> {
		co_await Logic();
	}, detached);

	// success
	return true;
}

void MainScene::Reset() {
	cmd.Emplace();
	totalDelta = 0;
	secs = 0;
	c.Reset();
	selfId = 0;
	self = nullptr;
	scene.Reset();
	frameBackups.Clear();
	frameBackupsFirstFrameNumber = 0;
}

void MainScene::update(float delta) {
	totalDelta += delta;																		// 累加时间池( 稳帧算法 )
	for (size_t i = 0; i < 3; i++) {
		c.Update();																				// 驱动 ioc
	}
}

awaitable<void> MainScene::Logic() {
	c.SetDomainPort("127.0.0.1", 12345);														// 初始化拨号地址
LabBegin:
	DrawInit();																					// 无脑重置一发
	Reset();
	co_await xx::Timeout(200ms);																// 睡 0.2 秒
	DrawDial();																					// 开始拨号
	if (auto r = co_await c.Dial()) {															// 域名解析并拨号. 失败就重来
		xx::CoutTN("Dial r = ", r);
		goto LabBegin;
	}

	c.SendPush<SS_C2S::Enter>();															// 发 enter
	if (auto o = co_await c.PopPush<SS_S2C::EnterResult>(5s)) {								// 等 enter result 包 ? 秒
		selfId = o->clientId;																// 等到: 存储自己的 id
	}
	else goto LabBegin;																		// 如果超时，重连

	if (auto o = co_await c.PopPush<SS_S2C::Sync>(5s)) {									// 等 sync 包 ? 秒
		scene = std::move(o->scene);														// 等到: 存储 scene
	}
	else goto LabBegin;																		// 如果超时，重连

	DrawPlay();																				// 开始游戏
	do {
		bool updated = false;
		while (auto o = c.TryPopPush()) {													// 继续处理 events 直到没有。尝试拿出一个收到的包
			if (!c.om.IsBaseOf<SS_S2C::Event>(o.typeId())) {								// 确保拿到的是 Event 基类
				c.om.CoutTN("receive a unhandled base type message: ", o);
				c.Reset();
				break;
			}
			auto&& e = o.ReinterpretCast<SS_S2C::Event>();									// 类型还原备用

			if (e->frameNumber > scene->frameNumber) {										// 看看要不要追帧
				xx::CoutTN("fast forward from ", scene->frameNumber, " to ", e->frameNumber);
				do {
					scene->Update();														// 追帧并备份
					Backup();
				} while (e->frameNumber == scene->frameNumber);								// 判断追帧次数
			} else if (e->frameNumber < scene->frameNumber) {								// 看看要不要回滚
				if (int r = Rollback(e->frameNumber)) goto LabBegin;						// 回滚。失败（网络太卡？已定位不到历史数据）则重新拨号
			}
				
			for (auto& cid : e->quits) {													// 处理玩家退出( 优先于 进入 )
				scene->shooters.erase(cid);													// 移除相关玩家对象
			}
				
			for (auto& s : e->enters) {														// 处理玩家进入
				auto r = scene->shooters.try_emplace(s->clientId, s);						// 将 shooters 添加到场景
				assert(r.second);	// 不应该失败
				s->scene = scene;															// 恢复引用关系啥的
				if (selfId == s->clientId) {												// 如果是自己，就存储其 ptr
					self = s;
				}
			}

				
			for (auto& c : e->css) {														// 处理玩家输入状态
				auto iter = scene->shooters.find(std::get<0>(c));							// 查找 shooter
				assert(iter != scene->shooters.end());	// 不应该找不到
				iter->second->cs = std::get<1>(c);											// 应用 control state
			}
				
			scene->Update();																// 更新场景( 追帧 )
			Backup();																		// 备份
			updated = true;
		}

		while (totalDelta > (1.f / 60.f)) {														// 开始消耗时间池的时间( 处理本地输入，转为网络包发出 )
			totalDelta -= (1.f / 60.f);															// 递减时间池( 稳帧算法 )

			scene->Update();																	// 更新场景( 自我演进 )
			Backup();																			// 备份
			updated = true;																		// 打更新标记( 触发 draw )

			// 缩放处理( 本地行为 )
			if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_Z] && zoom > 0.05f) {
				zoom -= 0.005f;
			}
			if (keyboards[(int)cocos2d::EventKeyboard::KeyCode::KEY_X] && zoom < 1.f) {
				zoom += 0.005f;
			}
			container->setScale(zoom);

			// todo: 自动 zoom ? 默认以当前 shooter 为中心点显示，当鼠标靠近屏幕边缘时，自动缩小显示并令 shooter 往相反方向偏移？( 本地行为 )

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
				// todo: 控制发包频率. 否则每秒发 60 个包，对于非局域网来说，很不科学
				// 如果限定到 1/10 秒一个控制指令，则需要想办法合并, 时间按本次操作开始时间点对齐
				// 将按键状态分为 点击 和 翻转 两种状态，对于 1/10 内按下又放开的操作，翻译为 点击。
				// 跨频率未放开，则视为 翻转（也就是和当前策略一样）
				// 鼠标位置跳跃问题，大概只能结合策划案来处理，约定 shooter 转身速度不能超过每秒多少角度
				// 这样就可以让 shooter 多用几帧来转身，最终面向鼠标，达到平滑插值的效果
				c.SendPush(cmd);
			}
		}
		// todo: 超时发送个防掉线包，避免 server kick

		if (updated) {
			scene->Draw();																		// 绘制场景
		}

		co_await xx::Timeout(0ms);																// 睡 1 帧
	} while (c);
	goto LabBegin;
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

	// 重新定位一下 self
	self = scene->shooters[selfId];

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
	for (auto& kv : shooters) {
		kv.second->Draw();
	}
	// todo: 调整 container 的 pos, 令 self 居中?
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
