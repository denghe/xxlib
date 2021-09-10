XY(int32_t const& x, int32_t const& y) : x(x), y(y) {}

XY operator+(XY const& v) const {
    return { x + v.x, y + v.y };
}

XY& operator+=(XY const& v) {
    x += v.x;
    y += v.y;
    return *this;
}

XY operator-(XY const& v) const {
    return { x - v.x, y - v.y };
}

XY& operator-=(XY const& v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

XY operator-() const {
    return { -x, -y };
}

XY operator*(int const& n) const {
    return { x * n, y * n };
}

XY& operator*=(int const& n) {
    x *= n;
    y *= n;
    return *this;
}

XY operator/(int const& n) const {
    return { x / n, y / n };
}

bool operator!=(XY const& v) const {
    return x != v.x || y != v.y;
}

#ifdef _WIN32
	operator cocos2d::Point() {
		return { (float)x, (float)y };
	}
#endif
