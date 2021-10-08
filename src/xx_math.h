#pragma once
#include <cstdint>
#include <array>
#include <cassert>

#ifndef _USE_MATH_DEFINES
#   define _USE_MATH_DEFINES
#endif
#include <math.h>


// 整数坐标系游戏常用函数/查表库。主要意图为确保跨硬件计算结果的一致性( 附带性能有一定提升 )

namespace xx {

	// 整数坐标
	template<typename T = int>
	struct XY {
		T x, y;

		XY() : x(0), y(0) {}
		XY(T const& x, T const& y) : x(x), y(y) {}
		XY(XY const& v) {
			memcpy(this, &v, sizeof(v));
		};
		XY& operator=(XY const& v) {
			memcpy(this, &v, sizeof(v));
			return *this;
		}

		// -x
		XY operator-() const {
			return { -x, -y };
		}

		// + - * /
		XY operator+(XY const& v) const {
			return { x + v.x, y + v.y };
		}
		XY operator-(XY const& v) const {
			return { x - v.x, y - v.y };
		}
		XY operator*(T const& n) const {
			return { x * n, y * n };
		}
		XY operator/(T const& n) const {
			return { x / n, y / n };
		}

		// += -= *= /=
		XY& operator+=(XY const& v) {
			x += v.x;
			y += v.y;
			return *this;
		}
		XY& operator-=(XY const& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}
		XY& operator*=(T const& n) {
			x *= n;
			y *= n;
			return *this;
		}
		XY operator/=(T const& n) {
			x /= n;
			y /= n;
			return *this;
		}

		// == !=
		bool operator==(XY const& v) const {
			if constexpr (std::is_integral_v<T>) {
				if constexpr (sizeof(T) == 4) {
					return *(uint64_t*)&x == *(uint64_t*)&v.x;
				} else if constexpr (sizeof(T) == 2) {
					return *(uint32_t*)&x == *(uint32_t*)&v.x;
				}
			} else {
				return x == v.x && y == v.y;
			}
		}
		bool operator!=(XY const& v) const {
			if constexpr (std::is_integral_v<T>) {
				if constexpr (sizeof(T) == 4) {
					return *(uint64_t*)&x != *(uint64_t*)&v.x;
				} else if constexpr (sizeof(T) == 2) {
					return *(uint32_t*)&x != *(uint32_t*)&v.x;
				}
			} else {
				return x != v.x || y != v.y;
			}
		}

		// zero check
		bool IsZero() const {
			if constexpr (std::is_integral_v<T>) {
				if constexpr (sizeof(T) == 4) {
					return *(uint64_t*)&x == 0;
				} else if constexpr (sizeof(T) == 2) {
					return *(uint32_t*)&x != 0;
				}
			} else {
				return x == T{} && y == T{};
			}
		}
	};

	const double pi2 = 3.14159265358979323846 * 2;

	// 设定计算坐标系 x, y 值范围 为 正负 table_xy_range
	const int table_xy_range = 2048, table_xy_range2 = table_xy_range * 2, table_xy_rangePOW2 =
		table_xy_range * table_xy_range;

	// 角度分割精度。256 对应 uint8_t，65536 对应 uint16_t, ... 对于整数坐标系来说，角度继续细分意义不大。增量体现不出来
	const int table_num_angles = 65536;
	using table_angle_element_type = uint16_t;

	// 查表 sin cos 整数值 放大系数
	const int64_t table_sincos_ratio = 10000;

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
	template<bool safeMode = true, typename T = int>
	table_angle_element_type GetAngleXY(T x, T y) noexcept {
		if constexpr (safeMode) {
			while (x < -table_xy_range || x >= table_xy_range || y < -table_xy_range || y >= table_xy_range) {
				x /= 8;
				y /= 8;
			}
		} else {
			assert(x >= -table_xy_range && x < table_xy_range&& y >= -table_xy_range && y < table_xy_range);
		}
		return table_angle[(y + table_xy_range) * table_xy_range2 + x + table_xy_range];
	}

	template<bool safeMode = true, typename T = int>
	table_angle_element_type GetAngleXYXY(T const& x1, T const& y1, T const& x2, T const& y2) noexcept {
		return GetAngleXY<safeMode>(x2 - x1, y2 - y1);
	}

	template<bool safeMode = true, typename Point1, typename Point2>
	table_angle_element_type GetAngle(Point1 const& from, Point2 const& to) noexcept {
		return GetAngleXY<safeMode>(to.x - from.x, to.y - from.y);
	}

	// 计算点旋转后的坐标
	template<typename P, typename T = decltype(P::x)>
	inline P Rotate(T const& x, T const& y, table_angle_element_type const& a) noexcept {
		auto s = (int64_t)table_sin[a];
		auto c = (int64_t)table_cos[a];
		return { (T)((x * c - y * s) / table_sincos_ratio), (T)((x * s + y * c) / table_sincos_ratio) };
	}
	template<typename P>
	inline P Rotate(P const& p, table_angle_element_type const& a) noexcept {
		static_assert(std::is_same_v<decltype(p.x), decltype(p.y)>);
		return Rotate<P, decltype(p.x)>(p.x, p.y, a);
	}

	// distance^2 = (x1 - x1)^2 + (y1 - y2)^2
	// return (r1^2 + r2^2 > distance^2)
	template<typename P>
	inline bool DistanceNear(int const& r1, int const& r2, P const& p1, P const& p2) {
		return r1 * r1 + r2 * r2 > (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
	}
}



// 抄自 qt 的 sin cos 函数。可以为输出到显卡的相关运算加速

namespace xx {

	const double qt_sine_table[256] = {
		double(0.0),
		double(0.024541228522912288),
		double(0.049067674327418015),
		double(0.073564563599667426),
		double(0.098017140329560604),
		double(0.1224106751992162),
		double(0.14673047445536175),
		double(0.17096188876030122),
		double(0.19509032201612825),
		double(0.2191012401568698),
		double(0.24298017990326387),
		double(0.26671275747489837),
		double(0.29028467725446233),
		double(0.31368174039889152),
		double(0.33688985339222005),
		double(0.35989503653498811),
		double(0.38268343236508978),
		double(0.40524131400498986),
		double(0.42755509343028208),
		double(0.44961132965460654),
		double(0.47139673682599764),
		double(0.49289819222978404),
		double(0.51410274419322166),
		double(0.53499761988709715),
		double(0.55557023301960218),
		double(0.57580819141784534),
		double(0.59569930449243336),
		double(0.61523159058062682),
		double(0.63439328416364549),
		double(0.65317284295377676),
		double(0.67155895484701833),
		double(0.68954054473706683),
		double(0.70710678118654746),
		double(0.72424708295146689),
		double(0.74095112535495911),
		double(0.75720884650648446),
		double(0.77301045336273699),
		double(0.78834642762660623),
		double(0.80320753148064483),
		double(0.81758481315158371),
		double(0.83146961230254524),
		double(0.84485356524970701),
		double(0.85772861000027212),
		double(0.87008699110871135),
		double(0.88192126434835494),
		double(0.89322430119551532),
		double(0.90398929312344334),
		double(0.91420975570353069),
		double(0.92387953251128674),
		double(0.93299279883473885),
		double(0.94154406518302081),
		double(0.94952818059303667),
		double(0.95694033573220894),
		double(0.96377606579543984),
		double(0.97003125319454397),
		double(0.97570213003852857),
		double(0.98078528040323043),
		double(0.98527764238894122),
		double(0.98917650996478101),
		double(0.99247953459870997),
		double(0.99518472667219682),
		double(0.99729045667869021),
		double(0.99879545620517241),
		double(0.99969881869620425),
		double(1.0),
		double(0.99969881869620425),
		double(0.99879545620517241),
		double(0.99729045667869021),
		double(0.99518472667219693),
		double(0.99247953459870997),
		double(0.98917650996478101),
		double(0.98527764238894122),
		double(0.98078528040323043),
		double(0.97570213003852857),
		double(0.97003125319454397),
		double(0.96377606579543984),
		double(0.95694033573220894),
		double(0.94952818059303667),
		double(0.94154406518302081),
		double(0.93299279883473885),
		double(0.92387953251128674),
		double(0.91420975570353069),
		double(0.90398929312344345),
		double(0.89322430119551521),
		double(0.88192126434835505),
		double(0.87008699110871146),
		double(0.85772861000027212),
		double(0.84485356524970723),
		double(0.83146961230254546),
		double(0.81758481315158371),
		double(0.80320753148064494),
		double(0.78834642762660634),
		double(0.7730104533627371),
		double(0.75720884650648468),
		double(0.74095112535495899),
		double(0.72424708295146689),
		double(0.70710678118654757),
		double(0.68954054473706705),
		double(0.67155895484701855),
		double(0.65317284295377664),
		double(0.63439328416364549),
		double(0.61523159058062693),
		double(0.59569930449243347),
		double(0.57580819141784545),
		double(0.55557023301960218),
		double(0.53499761988709715),
		double(0.51410274419322177),
		double(0.49289819222978415),
		double(0.47139673682599786),
		double(0.44961132965460687),
		double(0.42755509343028203),
		double(0.40524131400498992),
		double(0.38268343236508989),
		double(0.35989503653498833),
		double(0.33688985339222033),
		double(0.31368174039889141),
		double(0.29028467725446239),
		double(0.26671275747489848),
		double(0.24298017990326407),
		double(0.21910124015687005),
		double(0.19509032201612861),
		double(0.17096188876030122),
		double(0.1467304744553618),
		double(0.12241067519921635),
		double(0.098017140329560826),
		double(0.073564563599667732),
		double(0.049067674327417966),
		double(0.024541228522912326),
		double(0.0),
		double(-0.02454122852291208),
		double(-0.049067674327417724),
		double(-0.073564563599667496),
		double(-0.09801714032956059),
		double(-0.1224106751992161),
		double(-0.14673047445536158),
		double(-0.17096188876030097),
		double(-0.19509032201612836),
		double(-0.2191012401568698),
		double(-0.24298017990326382),
		double(-0.26671275747489825),
		double(-0.29028467725446211),
		double(-0.31368174039889118),
		double(-0.33688985339222011),
		double(-0.35989503653498811),
		double(-0.38268343236508967),
		double(-0.40524131400498969),
		double(-0.42755509343028181),
		double(-0.44961132965460665),
		double(-0.47139673682599764),
		double(-0.49289819222978393),
		double(-0.51410274419322155),
		double(-0.53499761988709693),
		double(-0.55557023301960196),
		double(-0.57580819141784534),
		double(-0.59569930449243325),
		double(-0.61523159058062671),
		double(-0.63439328416364527),
		double(-0.65317284295377653),
		double(-0.67155895484701844),
		double(-0.68954054473706683),
		double(-0.70710678118654746),
		double(-0.72424708295146678),
		double(-0.74095112535495888),
		double(-0.75720884650648423),
		double(-0.77301045336273666),
		double(-0.78834642762660589),
		double(-0.80320753148064505),
		double(-0.81758481315158382),
		double(-0.83146961230254524),
		double(-0.84485356524970701),
		double(-0.85772861000027201),
		double(-0.87008699110871135),
		double(-0.88192126434835494),
		double(-0.89322430119551521),
		double(-0.90398929312344312),
		double(-0.91420975570353047),
		double(-0.92387953251128652),
		double(-0.93299279883473896),
		double(-0.94154406518302081),
		double(-0.94952818059303667),
		double(-0.95694033573220882),
		double(-0.96377606579543984),
		double(-0.97003125319454397),
		double(-0.97570213003852846),
		double(-0.98078528040323032),
		double(-0.98527764238894111),
		double(-0.9891765099647809),
		double(-0.99247953459871008),
		double(-0.99518472667219693),
		double(-0.99729045667869021),
		double(-0.99879545620517241),
		double(-0.99969881869620425),
		double(-1.0),
		double(-0.99969881869620425),
		double(-0.99879545620517241),
		double(-0.99729045667869021),
		double(-0.99518472667219693),
		double(-0.99247953459871008),
		double(-0.9891765099647809),
		double(-0.98527764238894122),
		double(-0.98078528040323043),
		double(-0.97570213003852857),
		double(-0.97003125319454397),
		double(-0.96377606579543995),
		double(-0.95694033573220894),
		double(-0.94952818059303679),
		double(-0.94154406518302092),
		double(-0.93299279883473907),
		double(-0.92387953251128663),
		double(-0.91420975570353058),
		double(-0.90398929312344334),
		double(-0.89322430119551532),
		double(-0.88192126434835505),
		double(-0.87008699110871146),
		double(-0.85772861000027223),
		double(-0.84485356524970723),
		double(-0.83146961230254546),
		double(-0.81758481315158404),
		double(-0.80320753148064528),
		double(-0.78834642762660612),
		double(-0.77301045336273688),
		double(-0.75720884650648457),
		double(-0.74095112535495911),
		double(-0.724247082951467),
		double(-0.70710678118654768),
		double(-0.68954054473706716),
		double(-0.67155895484701866),
		double(-0.65317284295377709),
		double(-0.63439328416364593),
		double(-0.61523159058062737),
		double(-0.59569930449243325),
		double(-0.57580819141784523),
		double(-0.55557023301960218),
		double(-0.53499761988709726),
		double(-0.51410274419322188),
		double(-0.49289819222978426),
		double(-0.47139673682599792),
		double(-0.44961132965460698),
		double(-0.42755509343028253),
		double(-0.40524131400499042),
		double(-0.38268343236509039),
		double(-0.359895036534988),
		double(-0.33688985339222),
		double(-0.31368174039889152),
		double(-0.2902846772544625),
		double(-0.26671275747489859),
		double(-0.24298017990326418),
		double(-0.21910124015687016),
		double(-0.19509032201612872),
		double(-0.17096188876030177),
		double(-0.14673047445536239),
		double(-0.12241067519921603),
		double(-0.098017140329560506),
		double(-0.073564563599667412),
		double(-0.049067674327418091),
		double(-0.024541228522912448)
	};

	inline double qFastSin(double x) {
		int si = int(x * (0.5 * 256 / M_PI)); // Would be more accurate with qRound, but slower.
		double d = x - si * (2.0 * M_PI / 256);
		int ci = si + 256 / 4;
		si &= 256 - 1;
		ci &= 256 - 1;
		return qt_sine_table[si] + (qt_sine_table[ci] - 0.5 * qt_sine_table[si] * d) * d;
	}
	inline double qFastCos(double x) {
		int ci = int(x * (0.5 * 256 / M_PI)); // Would be more accurate with qRound, but slower.
		double d = x - ci * (2.0 * M_PI / 256);
		int si = ci + 256 / 4;
		si &= 256 - 1;
		ci &= 256 - 1;
		return qt_sine_table[si] - (qt_sine_table[ci] + 0.5 * qt_sine_table[si] * d) * d;
	}
}
