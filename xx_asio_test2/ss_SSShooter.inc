int Update();

#ifdef CC_STATIC
cocos2d::Sprite* body = nullptr;
cocos2d::Sprite* gun = nullptr;

void Draw();
~Shooter();
#endif
