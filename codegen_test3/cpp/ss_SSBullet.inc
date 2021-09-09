int Update();

#ifdef _WIN32
cocos2d::Sprite* body = nullptr;

void DrawInit();
void DrawUpdate();
void DrawDispose();
~Bullet();
#endif
