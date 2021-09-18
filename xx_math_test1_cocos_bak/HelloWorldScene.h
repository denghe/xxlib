#pragma once

#include "cocos2d.h"

namespace GameLogic {
	struct Scene;
}

class HelloWorld : public cocos2d::Scene
{
public:
	GameLogic::Scene* scene = nullptr;

	cocos2d::Node* container = nullptr;

    virtual bool init() override;

    void update(float delta) override;

	~HelloWorld();
};
