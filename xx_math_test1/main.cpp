#include "xx_string.h"
//#ifdef _WIN32
//#include <mimalloc-new-delete.h>
//#endif

// 计算直线的弧度
template<typename Point1, typename Point2>
float GetAngle(Point1 const& from, Point2 const& to) noexcept {
	if (from.x == to.x && from.y == to.y) return 0.0f;
	auto&& len_y = to.y - from.y;
	auto&& len_x = to.x - from.x;
	return atan2f(len_y, len_x);
}

// 计算距离
template<typename Point1, typename Point2>
float GetDistance(Point1 const& a, Point2 const& b) noexcept {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx * dx + dy * dy);
}

// 点围绕 0,0 为中心旋转 a 弧度   ( 角度 * (float(M_PI) / 180.0f) )
template<typename Point>
inline Point Rotate(Point const& pos, float const& a) noexcept {
	auto&& sinA = sinf(a);
	auto&& cosA = cosf(a);
	return Point{ pos.x * cosA - pos.y * sinA, pos.x * sinA + pos.y * cosA };
}



struct XY {
	int x, y;
};

/***********************************************************************************************/
// 整数坐标系游戏常用函数/查表库。主要意图为确保跨硬件计算结果的一致性( 附带性能有一定提升 )

static const double pi2 = 3.14159265358979323846264338328 * 2;

// 设定计算坐标系 x, y 值范围 为 正负 table_xy_range
static const int table_xy_range = 2048, table_xy_range2 = table_xy_range * 2, table_xy_rangePOW2 = table_xy_range * table_xy_range;

// 角度分割精度。256 对应 uint8_t，65536 对应 uint16_t, ... 对于整数坐标系来说，角度继续细分意义不大。增量体现不出来
static const int table_num_angles = 65536;
using table_angle_element_type = uint16_t;

// 查表 sin cos 整数值 放大系数
static const int64_t table_sincos_ratio = 100000;

// 构造一个查表数组，下标是 +- table_xy_range 二维坐标值。内容是以 table_num_angles 为切割单位的自定义角度值( 并非 360 或 弧度 )
inline std::array<table_angle_element_type, table_xy_range2* table_xy_range2> table_angle;

// 基于 table_num_angles 个角度，查表 sin cos 值 ( 通常作为移动增量 )
inline std::array<int, table_num_angles> table_sin;
inline std::array<int, table_num_angles> table_cos;


// 程序启动时自动填表
struct TableFiller {
	TableFiller() {
		// todo: 多线程计算提速
		for (int y = -table_xy_range; y < table_xy_range; ++y) {
			for (int x = -table_xy_range; x < table_xy_range; ++x) {
				auto idx = (y + table_xy_range) * table_xy_range2 + (x + table_xy_range);
				auto a = atan2((double)y, (double)x);
				if (a < 0) a += pi2;
				table_angle[idx] = (table_angle_element_type)(a / pi2 * table_num_angles);
			}
		}
		// fix same x,y
		table_angle[(0 + table_xy_range) * table_xy_range2 + (0 + table_xy_range)] = 0;

		for (int i = 0; i < table_num_angles; ++i) {
			auto s = sin((double)i / table_num_angles * pi2);
			table_sin[i] = (int)(s * table_sincos_ratio);
			auto c = cos((double)i / table_num_angles * pi2);
			table_cos[i] = (int)(c * table_sincos_ratio);
		}
	}
};
inline TableFiller tableFiller__;

// 传入坐标，返回角度值( 0 ~ table_num_angles )  safeMode: 支持大于查表尺寸的 xy 值 ( 慢几倍 )
template<bool safeMode = true>
table_angle_element_type GetAngleXY(int x, int y) noexcept {
	if constexpr (safeMode) {
		while (x < -table_xy_range || x >= table_xy_range || y < -table_xy_range || y >= table_xy_range) {
			x /= 8;
			y /= 8;
		}
	}
	else {
		assert(x >= -table_xy_range && x < table_xy_range && y >= -table_xy_range && y < table_xy_range);
	}
	return table_angle[(y + table_xy_range) * table_xy_range2 + x + table_xy_range];
}
template<bool safeMode = true>
table_angle_element_type GetAngleXYXY(int const& x1, int const& y1, int const& x2, int const& y2) noexcept {
	return GetAngleXY<safeMode>(x2 - x1, y2 - y1);
}
template<bool safeMode = true, typename Point1, typename Point2>
table_angle_element_type GetAngle(Point1 const& from, Point2 const& to) noexcept {
	return GetAngleXY<safeMode>(to.x - from.x, to.y - from.y);
}

// 计算点旋转后的坐标
inline XY Rotate(XY const& p, table_angle_element_type const& a) noexcept {
	auto s = (int64_t)table_sin[a];
	auto c = (int64_t)table_cos[a];
	return { (int)(p.x * c / table_sincos_ratio - p.y * s / table_sincos_ratio), (int)(p.x * s / table_sincos_ratio + p.y * c / table_sincos_ratio) };
}

/***********************************************************************************************/

int main() {
	{
		auto secs = xx::NowEpochSeconds();
		double count = 0;
		for (int k = 0; k < 50; ++k) {
			for (int y = -1024; y < 1024; ++y) {
				for (int x = -1024; x < 1024; ++x) {
					count += atan2f((float)y, (float)x);
				}
			}
		}
		xx::CoutN(count);
		xx::CoutN(xx::NowEpochSeconds() - secs);
	}

	auto secs = xx::NowEpochSeconds();
	int64_t count = 0;
	for (int k = 0; k < 50; ++k) {
		for (int y = -1024; y < 1024; ++y) {
			for (int x = -1024; x < 1024; ++x) {
				//count += table_angle[(y + 1024) * 2048 + x + 1024];
				count += GetAngleXY<true>(x, y);
			}
		}
	}
	xx::CoutN(count);
	xx::CoutN(xx::NowEpochSeconds() - secs);

	//for(auto& o : table_sin) {
	//    xx::Cout(o, " ");
	//}
	//xx::CoutN();
	//for(auto& o : table_cos) {
	//    xx::Cout(o, " ");
	//}
	//xx::CoutN();
	return 0;
}
