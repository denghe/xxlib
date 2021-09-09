XY(int32_t const& x, int32_t const& y) : x(x), y(y) {}

#ifdef _WIN32
	operator cocos2d::Point() {
		return { (float)x, (float)y };
	}
#endif
