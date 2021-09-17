#include "xx_math.h"
#include "xx_ptr.h"
#include "xx_string.h"

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

// 基础坐标
struct XY {
	int x = 0, y = 0;
};

// 场景( 预声明 )
struct Scene;

// 场景内对象基类
struct Obj {
	// 指向所在场景( 生命周期确保该指针必然有效 )
	Scene* scene;
	// 所在 grid 下标. 放入 scene->grid 后，该值不为 -1
	int gridIndex = -1;
	// 当前坐标
	XY xy;

	// 禁不传 scene, 禁 copy
	Obj() = delete;
	Obj(Obj const&) = delete;
	Obj& operator=(Obj const&) = delete;

	// 默认构造
	Obj(Scene* scene) : scene(scene) {}
	// 如果有放入 scene->grid，则自动移除
	virtual ~Obj();
	// 如果有放入 scene->grid，修改过 xy 后 需要调用这个函数来同步索引
	void SyncGrid();
	// 基础逻辑更新
	virtual size_t Update() = 0;
};

// 场景
struct Scene {
	int frameNumber = 0;
	std::vector<xx::Shared<Obj>> objs;
	xx::Grid2d<Obj*> grid;

	Scene(Scene const&) = delete;
	Scene& operator=(Scene const&) = delete;
	Scene() {
		objs.reserve(100000);
		grid.Init(5000, 5000, 100000);
	}

	size_t Update() {
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

Obj::~Obj() {
	if (gridIndex != -1) {
		scene->grid.Remove(gridIndex);
		gridIndex = -1;
	}
}

void Obj::SyncGrid() {
	assert(gridIndex != -1);
	scene->grid.Update(gridIndex, xy.x, xy.y);
}

struct Foo : Obj {
};

int main() {
	Scene scene;

	std::cin.get();
	return 0;
}
