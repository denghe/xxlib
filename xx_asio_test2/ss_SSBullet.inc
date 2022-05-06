virtual int Update() {
	assert(false);
	return -1;
};

#ifdef CC_STATIC
cocos2d::Sprite* body = nullptr;

void Draw();
~Bullet();
#endif
