#pragma once
#include "cocos2d.h"
#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <ss.h>
#include <xx_asiokcpclient.h>

class MainScene : public cocos2d::Scene {
public:
	// ��ʼ��
	bool init() override;

	// cocos �ײ�֡����
	void update(float delta) override;

	// ������ʾ����, ��� �Լ� ����
	float zoom = 0.5f;
	cocos2d::Node* container = nullptr;
	cocos2d::Node* ui = nullptr;
	cocos2d::Sprite* cursor = nullptr;

	// ����ӳ��( ÿ֡���жϲ���䣬�γ� ControlState )
	cocos2d::Point mousePos;
	std::array<bool, 9> mouseKeys;
	std::array<bool, 166> keyboards;

	// ���ݵ�ǰ����״̬. ÿ�θ����������һ���µģ���������Ƚϣ���һ�¾ͷ���
	xx::Shared<SS_C2S::Cmd> cmd;

	// ����Э��( ���� ����, UI �� )
	double totalDelta = 0;
	double secs = 0;
	xx::AsioKcpClient c;
	bool synced = false;
	bool playing = false;
	xx::ObjManager om;
	int lineNumber = 0;
	int Update();

	// Э������ GUI ����
	void DrawInit();
	void DrawResolve();
	void DrawDial();
	void DrawPlay();

	// ��Ϸ������
	xx::Shared<SS::Scene> scene;

	// ÿһ֡ update ��ı��ر���, ���յ��� events С�� scene.frameNumber ʱ��������ع�
	std::deque<xx::Data> frameBackups;

	// ÿ֡���� �� ��һ֡ �ı��
	int frameBackupsFirstFrameNumber = 0;
};




//struct XY {
//	int x = 0, y = 0;
//	operator cocos2d::Point() {
//		return { (float)x, (float)y };
//	}
//};
//
//class MainScene;
//struct Bullet;
//struct Shooter {
//	Shooter(MainScene* mainScene, XY pos);
//	~Shooter();
//	MainScene* mainScene;
//	cocos2d::Sprite* body;
//	cocos2d::Sprite* gun;
//	float bodyAngle = 0.f;
//	XY pos;
//	int moveDistancePerFrame = 10;
//
//	XY aimPos;
//	bool moveLeft = false;
//	bool moveRight = false;
//	bool moveUp = false;
//	bool moveDown = false;
//	bool button1 = false;
//	bool button2 = false;
//	std::vector<std::shared_ptr<Bullet>> bullets;
//	int Update();
//};
//
//struct Bullet {
//	Bullet(Shooter* shooter, XY pos, XY inc, int life);
//	~Bullet();
//	Shooter* shooter;
//	MainScene* mainScene;
//	int life;
//	XY pos, inc;
//	// target? ����?
//	cocos2d::Sprite* body;
//	int Update();
//};