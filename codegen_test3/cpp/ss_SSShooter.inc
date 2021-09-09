int Update();

#ifdef _WIN32
cocos2d::Sprite* body = nullptr;
cocos2d::Sprite* gun = nullptr;

void DrawInit();
void DrawUpdate();
void DrawDispose();
~Shooter();
#endif
