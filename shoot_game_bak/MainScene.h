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
	// 初始化
	bool init() override;

	// cocos 底层帧驱动
	void update(float delta) override;

	// 基础显示容器, 光标 以及 缩放
	float zoom = 0.5f;
	cocos2d::Node* container = nullptr;
	cocos2d::Node* ui = nullptr;
	cocos2d::Sprite* cursor = nullptr;

	// 输入映射( 每帧会判断并填充，形成 ControlState )
	cocos2d::Point mousePos;
	std::array<bool, 9> mouseKeys;
	std::array<bool, 166> keyboards;

	// 备份当前控制状态. 每次根据输入产生一个新的，并和这个比较，不一致就发包
	xx::Shared<SS_C2S::Cmd> cmd;

	// 主线协程( 驱动 网络, UI 等 )
	double totalDelta = 0;
	double secs = 0;
	xx::AsioKcpClient c;
	bool synced = false;
	bool playing = false;
	xx::ObjManager om;
	int lineNumber = 0;
	int Update();

	// 协程配套 GUI 交互
	void DrawInit();
	void DrawResolve();
	void DrawDial();
	void DrawPlay();

	// 游戏上下文
	xx::Shared<SS::Scene> scene;

	// 每一帧 update 后的本地备份, 当收到的 events 小于 scene.frameNumber 时，从这里回滚
	std::deque<xx::Data> frameBackups;

	// 每帧备份 的 第一帧 的编号
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
//	// target? 跟踪?
//	cocos2d::Sprite* body;
//	int Update();
//};