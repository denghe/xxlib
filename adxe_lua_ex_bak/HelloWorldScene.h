#pragma once
#include "cocos2d.h"
#include <xx_coro.h>

class HelloWorld : public cocos2d::Scene {
public:
    HelloWorld();

    virtual bool init() override;

    xx::Coros coros;
    void update(float delta) override;
};
