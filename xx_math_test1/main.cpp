#ifdef _WIN32MIMALLIC
#include <mimalloc-new-delete.h>
#endif
#include "xx_math.h"
#include "xx_ptr.h"
#include "xx_string.h"
#include "xx_randoms.h"


/*
(0,0)                      grid
┌───────────────────────────────────────────────────────┐
│                          map                          │
│   ┌───────────────────────────────────────────────┐   │
│   │                                               │   │
│   │          x                                    │   │
│   │    x           screen           x             │   │
│   │          ┌─────────────────┐            x     │   │
│   │          │                 │                  │   │
│   │          │    x            │  x               │   │
│   │          │        O        │                  │   │
│   │          │              x  │                  │   │
│   │         x│                 │          x       │   │
│   │          └─────────────────┘                  │   │
│   │    x                             x            │   │
│   │               x                               │   │
│   │                      x                        │   │
│   │                                               │   │
│   │           x                 x           x     │   │
│   │                  x                            │   │
│   │                                              x│   │
│   └───────────────────────────────────────────────┘   │
│                                                       │
└───────────────────────────────────────────────────────┘

解释：x 怪物    O 玩家    screen 屏幕显示区域    map 游戏地图    grid 格子系统坐标覆盖范围
(0,0) 表示坐标原点. 对于 格子系统 / 游戏地图 来说，原点在左上角，而对于 opengl 啥的来讲，原点在左下角, 显示需 y 取反

如图, 怪物 零散分布在 游戏地图 中, 玩家 通常位于 屏幕 中心位置, 但整个 屏幕 也只是显示 地图的冰山一角
最后，为确保 格子系统 正常工作，地图 的物理数据 实际上 远大于 逻辑数据区域( 要求计算的所有数据范围都要在格子内 )
	这种设计是为了减少边缘处的 if 是否有格子的判断, 以提升执行效率

具体假设:
	屏幕 1280 * 720      ( 常见尺寸 )
	对象 100 * 100       ( 最多数量的对象的典型尺寸, 或者是去掉极端数据的最大值 )
	地图 4000 * 2000
	格子 ?????

下面来具体推算 格子尺寸以及分割参数：
	对于 格子系统 来说，对象 最好可以完整的装到一格中( 来自某物理 engine 的经验数据 )
	这样有 2 个好处：
		1. 移动剧烈时，产生跨格子的行为，不是太频繁
		2. 查找相邻时，大部分时候，关注 9宫 范围内就满足需求了，不至于筛出太多不相干的

	假设 怪物占地不重叠( 这样才有利于均匀分布到多个格子，也更符合人类常识 )
	怪物数量密集，铺满屏幕. 粗略计算，一地图，能铺 40 * 20 = 800 多只。

	由此可得到 grid 单元格直径 和 cap 参数，100, 800
	进一步的，由于需要确保边缘查询 9宫 不会超出 grid 数组下标，4个边缘各多空一格。
	地图 4边 留空，得到 4200 * 2200，游戏对象从 100, 100 开始分布，占据 x = 100 ~ 4100, y = 100 ~ 2100 的数据范围
	地图扩展后的实际尺寸，除以 100, 得到行列数

结果:
	屏幕 不变
	对象 100直径
	地图 4200宽 * 2200高
	格子 22行, 42列, 800预留, 100格子直径( 这个数据在 grid 系统外体现，要进一步包装换算 )
	考虑到整数除法性能，尺寸向 2 的幂 靠拢，地图按对象个数换算尺寸，继续优化

最终结果:
	屏幕 通过 scale 缩放来保持显示比例
	对象 128直径, 横向铺40只，纵向铺20只，共800只
	地图 128 * 2 + 128 * 40 只, 128 * 2 + 128 * 20 只 = 5376 * 2816
	格子 22行, 42列, 800预留, 128格子直径

理论上讲，格子的 列数，也可以向 2 的幂 靠拢，可用位运算 替代乘除法 提升换算性能。但需考虑额外空间带来的搜索的无效性访问问题

至于怪物或玩家的 子弹，围墙石头等刚体啥的，可以用类似参数，再建几套 grid 数据。
*/






/*****************************************************************************************************/
/*****************************************************************************************************/

namespace xx {

	// 对象管理器( 预声明 )
	struct ItemMgr;

	// 场景内对象基类
	struct Item {
		// 指向所在场景( 生命周期确保该指针必然有效 )
		ItemMgr* im;
		// 所在 grid 下标. 放入 im->grid 后，该值不为 -1
		int gridIndex = -1;
		// 当前坐标
		XY xy;

		// 禁不传 im, 禁 copy
		Item() = delete;
		Item(Item const&) = delete;
		Item& operator=(Item const&) = delete;

		// 默认构造( 派生类通常 using Item::Item 直接继承 )
		// 派生类需自行初始化 xy 并 gridIndex = im->grid.Add(xy.x / nnnn, xy.y / nnnn, this);
		// 中途同步用 im->grid.Update(gridIndex, xy.x / nnnn, xy.y / nnnn);
		Item(ItemMgr* im) : im(im) {}

		// 如果有放入 im->grid，则自动移除
		virtual ~Item();

		// 基础逻辑更新
		virtual size_t Update() = 0;
	};

	// 对象管理器
	struct ItemMgr {

		// 格子系统
		Grid2d<Item*> grid;

		// 对象容器 需要在 格子系统 的下方，确保 先 析构，这样才能正确的从 格子系统 移除
		std::vector<Shared<Item>> objs;

		ItemMgr() = default;
		ItemMgr(ItemMgr const&) = delete;
		ItemMgr& operator=(ItemMgr const&) = delete;

		virtual size_t Update() {
			for (int i = (int)objs.size() - 1; i >= 0; --i) {
				auto& o = objs[i];
				if (o->Update()) {
					if (auto n = (int)objs.size() - 1; i < n) {
						o = std::move(objs[n]);
					}
					objs.pop_back();
				}
			}
			return objs.size();
		}
	};

	inline Item::~Item() {
		if (gridIndex != -1) {
			assert(im->grid[gridIndex].value == this);
			im->grid.Remove(gridIndex);
			gridIndex = -1;
		}
	}

}


/*****************************************************************************************************/
/*****************************************************************************************************/

namespace GameLogic {

	// 下列代码模拟 一组 圆形对象, 位于 2d 空间，一开始挤在一起，之后物理随机排开, 直到静止。统计一共要花多长时间。
	// 数据结构方面不做特殊优化，走正常 oo 风格
	// 要对比的是 用 或 不用 grid 系统的效率


	// 模拟配置. 格子尺寸, grid 列数均为 2^n 方便位运算
	static const int gridWidth = 64;
	static const int gridHeight = 64;
	static const int gridDiameter = 128;
	//static const int gridWidth = 128;
	//static const int gridHeight = 128;
	//static const int gridDiameter = 64;

	static const int mapMinX = 128;				// >=
	static const int mapMaxX = 128 * 63 - 1;	// <=
	static const int mapMinY = 128;
	static const int mapMaxY = 128 * 63 - 1;
	static const xx::XY mapCenter = { (mapMinX + mapMaxX) / 2, (mapMinY + mapMaxY) / 2 };

	// Foo 的半径 / 直径 / 移动速度( 每帧移动单位距离 ) / 邻居查找个数
	static const int fooRadius = 50;
	static const int fooDiameter = fooRadius * 2;
	static const int fooSpeed = 5;
	static const int fooFindNeighborLimit = 20;



	struct Scene;
	struct Foo : xx::Item {
		using xx::Item::Item;
		int radius = fooRadius;
		int speed = fooSpeed;
#ifdef CC_STATIC
		cocos2d::Sprite* body = nullptr;
#endif
		Foo(Scene* scene, xx::XY const& xy);
		~Foo();
		Scene& scene();
		size_t Update() override;
	};

	struct Scene : xx::ItemMgr {
		using xx::ItemMgr::ItemMgr;

#ifdef CC_STATIC
		// 指向 cocos 显示层
		HelloWorld* hw = nullptr;
#endif

		// 随机数一枚, 用于对象完全重叠的情况下产生一个移动方向
		xx::Random4 rnd;

		// 每帧统计还有多少个对象正在移动
		int count = 0;

#ifdef CC_STATIC
		Scene(int const& fooCount, HelloWorld* const& hw) : hw(hw) {
#else
		Scene(int const& fooCount) {
#endif
			// 初始化容器
			grid.Init(gridWidth, gridHeight, fooCount);
			objs.reserve(fooCount);

			// 直接构造出 n 个 Foo
			for (int i = 0; i < fooCount; i++) {
				objs.emplace_back(xx::Make<Foo>(this, mapCenter));
			}
		}

		size_t Update() override {
			count = 0;
			return xx::ItemMgr::Update();
		}
	};

	inline Scene& Foo::scene() {
		return *(Scene*)im;
	}

	inline Foo::Foo(Scene* scene, xx::XY const& xy) : xx::Item(scene) {
		this->xy = xy;
		gridIndex = scene->grid.Add(xy.x / gridDiameter, xy.y / gridDiameter, this);
	}

	inline size_t Foo::Update() {
		// 备份坐标
		auto xy = this->xy;

		// 判断周围是否有别的 Foo 和自己重叠，取前 n 个，根据对方和自己的角度，结合距离，得到 推力矢量，求和 得到自己的前进方向和速度。
		// 最关键的是最终移动方向。速度需要限定最大值和最小值。如果算出来矢量为 0，那就随机角度正常速度移动。
		// 移动后的 xy 如果超出了 地图边缘，就硬调整. 

		// 查找 n 个邻居
		int limit = fooFindNeighborLimit;
		int crossNum = 0;
		xx::XY v;
		im->grid.LimitFindNeighbor(limit, gridIndex, [&](auto o) {
			assert(o != (Item*)this);
			// 类型转换下方便使用
			auto& f = *(Foo*)o;
			// 如果圆心完全重叠，则不产生推力
			if (f.xy == this->xy) {
				++crossNum;
				return;
			}
			// 准备判断是否有重叠. r1* r1 + r2 * r2 > (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y)
			auto r1 = f.radius;
			auto r2 = this->radius;
			auto r12 = r1 * r1 + r2 * r2;
			auto p1 = f.xy;
			auto p2 = this->xy;
			auto p12 = (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
			// 有重叠
			if (r12 > p12) {
				// 计算 f 到 this 的角度( 对方产生的推力方向 ). 
				auto a = xx::GetAngle<(gridDiameter * 2 >= 1024)>(p1, p2);
				// 重叠的越多，推力越大?
				auto inc = xx::Rotate(xx::XY{ this->speed * r12 / p12, 0 }, a);
				v += inc;
				++crossNum;
			}
			});

		if (crossNum) {
			// 如果 v == 0 那就随机角度正常速度移动
			if (v.IsZero()) {
				auto a = scene().rnd.Next() % xx::table_num_angles;
				auto inc = xx::Rotate(xx::XY{ speed, 0 }, a);
				xy += inc;
			}
			// 根据 v 移动
			else {
				auto a = xx::GetAngleXY(v.x, v.y);
				auto inc = xx::Rotate(xx::XY{ speed * std::max(crossNum, 5), 0 }, a);
				xy += inc;
			}

			// 边缘限定( 当前逻辑有重叠才移动，故边界检测放在这里 )
			if (xy.x < mapMinX) xy.x = mapMinX;
			else if (xy.x > mapMaxX) xy.x = mapMaxX;
			if (xy.y < mapMinY) xy.y = mapMinY;
			else if (xy.y > mapMaxY) xy.y = mapMaxY;
		}

		// 变更检测与同步
		if (xy != this->xy) {
			++scene().count;
			this->xy = xy;
			assert(gridIndex != -1);
			im->grid.Update(gridIndex, xy.x / gridDiameter, xy.y / gridDiameter);
		}

#ifdef CC_STATIC
		// 绘制
		if (!body) {
			body = cocos2d::Sprite::create("b.png");
			//body->setScale(1.25);
			scene().hw->container->addChild(body);
		}
		body->setPosition(xy.x, xy.y);
#endif

		return 0;
	}

	inline Foo::~Foo() {
#ifdef CC_STATIC
		if (body) {
			body->removeFromParent();
			body = nullptr;
		}
#endif
	}
}

int main() {
	for (size_t i = 0; i < 20; i++)	{
		GameLogic::Scene scene(5000);
		auto n = 0;
		auto secs = xx::NowEpochSeconds();
		do {
			scene.Update();
			//if (n != scene.count) {
			//	xx::CoutN("count = ", scene.count);
			//	n = scene.count;
			//}
		} while (scene.count);
		xx::CoutN("secs = ", xx::NowEpochSeconds() - secs);
	}

	std::cin.get();
	return 0;
}
