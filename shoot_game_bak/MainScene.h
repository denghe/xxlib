#pragma once
#include "cocos2d.h"
#include <array>
#include <optional>
#include <vector>
#include <memory>

class MainScene;
struct Bullet;
struct Shooter {
	Shooter(MainScene* mainScene, cocos2d::Point pos);
	~Shooter();
	MainScene* mainScene;
	cocos2d::Sprite* body;
	cocos2d::Sprite* gun;
	float bodyAngle = 0.f;
	cocos2d::Point pos;
	float moveDistancePerFrame = 10;

	cocos2d::Point aimPos;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveUp = false;
	bool moveDown = false;
	bool button1 = false;
	bool button2 = false;
	std::vector<std::shared_ptr<Bullet>> bullets;
	int Update();
};

struct Bullet {
	Bullet(Shooter* shooter, cocos2d::Point pos, cocos2d::Point inc, int life);
	~Bullet();
	Shooter* shooter;
	MainScene* mainScene;
	int life;
	cocos2d::Point pos, inc;
	// target? ¸ú×Ù?
	cocos2d::Sprite* body;
	int Update();
};

class MainScene : public cocos2d::Scene {
public:
	bool init() override;
	void update(float delta) override;

	float zoom = 1.0f;
	cocos2d::Node* container = nullptr;
	cocos2d::Sprite* cursor = nullptr;

	float totalDelta = 0.f;
	cocos2d::Point mousePos;
	std::array<bool, 9> mouseKeys;
	std::array<bool, 166> keyboards;

	std::shared_ptr<Shooter> shooter;
};
