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
	// �����շ�
	c.Update();

	// frame limit
	totalDelta += delta;
	while (totalDelta > (1.f / 60.f)) {
		totalDelta -= (1.f / 60.f);

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
			// todo: ���Ʒ���Ƶ��. ����ÿ�뷢 60 ���������ڷǾ�������˵���ܲ���ѧ
			// ����޶��� 1/10 ��һ������ָ�����Ҫ��취�ϲ�, ʱ�䰴���β�����ʼʱ������
			// ������״̬��Ϊ ��� �� ��ת ����״̬������ 1/10 �ڰ����ַſ��Ĳ���������Ϊ �����
			// ��Ƶ��δ�ſ�������Ϊ ��ת��Ҳ���Ǻ͵�ǰ����һ����
			// ���λ����Ծ���⣬���ֻ�ܽ�ϲ߻���������Լ�� shooter ת���ٶȲ��ܳ���ÿ����ٽǶ�
			// �����Ϳ����� shooter ���ü�֡��ת������������꣬�ﵽƽ����ֵ��Ч��
			c.Send(cmd);
		}
	}

	// todo: ��ʱ���͸������߰������� server kick

	// �����շ�( ����һ��. ��������ղ��� )
	c.Update();
}

void MainScene::Reset() {
	cmd.Emplace();
	totalDelta = 0;
	secs = 0;
	c.Reset();
	ok = false;
	playing = false;
	selfId = 0;
	self = nullptr;
	scene.Reset();
	frameBackups.Clear();
	frameBackupsFirstFrameNumber = 0;
}

int MainScene::Update() {
	COR_BEGIN;

	// ��ʼ�����ŵ�ַ
	c.SetDomainPort("192.168.1.95", 12345);

LabBegin:

	DrawInit();
	// ��������һ��
	Reset();

	// ˯ 0.2 ��
	secs = xx::NowEpochSeconds() + 0.2;
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

	// �� 10 ��, ���������ɾ�ֹͣ�ȴ�
	secs = xx::NowEpochSeconds() + 10;
	do {
		COR_YIELD;
		if (!c.Busy()) break;
	} while (secs > xx::NowEpochSeconds());

	// û����: ����
	if (!c.Alive()) goto LabBegin;


	// �� ���� ���󡣷����� ���� EnterResult + Sync( ����ɹ��Ļ� )��
	c.Send(xx::Make<SS_C2S::Enter>());

	// �� 5 ��, ���û���յ� EnterResult ����������
	secs = xx::NowEpochSeconds() + 5;
	ok = false;
	do {
		COR_YIELD;

		// ���Դ��հ����л�ȡ��
		xx::ObjBase_s o;
		if (c.TryGetPackage(o)) {
			// ���Ϊ �� �� ���� sync �� ����������
			if (!o || o.typeId() != xx::TypeId_v<SS_S2C::EnterResult>) goto LabBegin;
			auto&& m = o.ReinterpretCast<SS_S2C::EnterResult>();
			selfId = m->clientId;
			assert(selfId);
			ok = true;
			break;
		}

	} while (secs > xx::NowEpochSeconds());
	if (!ok) goto LabBegin;

	// �� 5 ��, ���û���յ� sync ����������
	secs = xx::NowEpochSeconds() + 5;
	ok = false;
	do {
		COR_YIELD;

		// ���Դ��հ����л�ȡ��
		xx::ObjBase_s o;
		if (c.TryGetPackage(o)) {
			// ���Ϊ �� �� ���� sync �� ����������
			if (!o || o.typeId() != xx::TypeId_v<SS_S2C::Sync>) goto LabBegin;
			auto&& sync = o.ReinterpretCast<SS_S2C::Sync>();
			scene = std::move(sync->scene);
			ok = true;
			break;
		}

	} while (secs > xx::NowEpochSeconds());
	if (!ok) goto LabBegin;

	DrawPlay();
	// ��ʼ��Ϸ��������߾�����
	playing = true;
	do {
		COR_YIELD;

		if (!c.receivedPackages.empty()) {

			// �������� events ֱ��û��
			xx::ObjBase_s o;

			// ���Դ��հ����л�ȡ��
			while (c.TryGetPackage(o)) {
				auto tid = o.typeId();

				// ����Ӧ���� Event ����
				assert(o && c.om.IsBaseOf<SS_S2C::Event>(tid));
				auto&& e = o.ReinterpretCast<SS_S2C::Event>();

				// ����Ҫ��Ҫ׷֡
				if (e->frameNumber > scene->frameNumber) {
					xx::CoutTN("fast forward from ", scene->frameNumber, " to ", e->frameNumber);
					do {
						// ׷֡������
						scene->Update();
						Backup();
					} while (e->frameNumber == scene->frameNumber);

					// ����Ҫ��Ҫ�ع�
				} else if (e->frameNumber < scene->frameNumber) {
					// �ع�������Ҫ���š�ʧ�ܣ�����̫�����Ѷ�λ������ʷ���ݣ������²���
					if (int r = Rollback(e->frameNumber)) goto LabBegin;
				}

				// ��������˳�
				for (auto& cid : e->quits) {
					scene->shooters.erase(cid);
				}

				// ������ҽ���
				for (auto& s : e->shooters) {
					// �� shooters ��ӵ�����
					auto cid = s->clientId;
					auto r = scene->shooters.try_emplace(cid, s);
					assert(r.second);
					s->scene = scene;

					// �� self
					if (selfId == cid) {
						self = s;
					}
				}

				// ���� shooters ��Ӧ�� cs
				for (auto& c : e->css) {
					auto iter = scene->shooters.find(std::get<0>(c));
					assert(iter != scene->shooters.end());
					iter->second->cs = std::get<1>(c);
				}

				// ���³���������
				scene->Update();
				Backup();
			}
		} else {
			// ���³���������
			scene->Update();
			Backup();
		}
		scene->Draw();

	} while (c.Alive());
	goto LabBegin;

	COR_END;
}

void MainScene::Backup() {
	// �״α���: ��¼֡���
	if (frameBackups.Empty()) {
		frameBackupsFirstFrameNumber = scene->frameNumber;
	} else {
		// ������г����޶����Ⱦ� pop һ����ͬ�� [0] �� frameNumber
		if (frameBackups.Count() == maxBackupCount) {
			frameBackups.Pop();
			++frameBackupsFirstFrameNumber;
		}
	}
	// ����
	auto& d = frameBackups.Emplace();
	c.om.WriteTo(d, scene);

	// �˲�
	assert(scene->frameNumber + 1 == frameBackupsFirstFrameNumber + frameBackups.Count());
}

int MainScene::Rollback(int const& frameNumber) {
	xx::CoutTN("rollback from ", scene->frameNumber, " to ", frameNumber);

	// �˲�
	assert(frameNumber < frameBackupsFirstFrameNumber + frameBackups.Count());

	// ���������Χ�ͷ���ʧ��( ������ )
	if (frameNumber < frameBackupsFirstFrameNumber) return -1;

	// �����Ŀ��֡���±�
	auto idx = frameNumber - frameBackupsFirstFrameNumber;

	// ������Ų����
	auto d = std::move(frameBackups[idx]);

	// �ع�( �൱���յ� sync )
	c.om.ReadFrom(d, scene);

	// ���¶�λһ�� self
	self = scene->shooters[selfId];

	// ��ն��У��� d ���룬ͬ�� [0] �� frameNumber
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

	// todo: ���� container �� pos, �� self ����?

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
