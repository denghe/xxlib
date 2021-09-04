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
	cocos2d::Point pos;
	bool moveLeft = false;
	bool moveRight = false;
	bool moveUp = false;
	bool moveDown = false;
	std::optional<cocos2d::Point> touchPos;
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

	// todo: texture cache

	float totalDelta = 0.f;
	std::optional<cocos2d::Point> touchPos;
	std::array<bool, 166> keys;

	std::shared_ptr<Shooter> shooter;
};
