#include "xx_math.h"
#include "xx_ptr.h"
#include "xx_string.h"

struct XY {
	int x, y;
};
struct Foo {
	XY xy;

	xx::Grid2d<Foo*>* g;
	int idx;

	void SetXY(XY const& xy_) {
		xy = xy_;
		g->Update(idx, xy_.x, xy_.y);
	}

	~Foo() {
		g->Remove(idx);
	}
};

int main() {
	xx::Grid2d<Foo*> g(5000, 5000);
	std::vector<xx::Shared<Foo>> foos;

	auto NewFoo = [&](int x, int y) {
		auto&& f = foos.emplace_back().Emplace();
		f->xy = { x, y };
		f->g = &g;
		f->idx = g.Add(f->xy.x, f->xy.y, &*f);
	};
	NewFoo(1, 1);
	NewFoo(4, 4);
	NewFoo(2, 2);
	NewFoo(3, 3);
	for (size_t i = 0; i < 100; i++) {
		NewFoo(1, 1);
	}

	xx::CoutN("foos.size() = ", foos.size());
	xx::CoutN("g.Count() = ", g.Count());

	foos[1]->SetXY({ 1, 1 });

	for (size_t j = 0; j < 10; j++) {
		auto secs = xx::NowEpochSeconds();
		int n;
		for (size_t i = 0; i < 10000000; i++) {
			n = 0;
			g.AnyNeighbor(1, 1, [&](Foo* const& p) {
				++n;
			});
		}
		xx::CoutN("n = ", n);
		xx::CoutN("secs = ", xx::NowEpochSeconds()-secs);
	}


	for (auto& f : foos) {
		xx::CoutN("f->idx = ", f->idx);
	}
	foos.clear();

	xx::CoutN("foos.size() = ", foos.size());
	xx::CoutN("g.Count() = ", g.Count());

	// todo: 模拟一批 foo 更新坐标
	// 令 foos 均匀分布在 grid. 执行范围检索。

	// todo2: 用半径查询，但是格子向外矩形扩散。不需要填充，只需要逐个枚举。
	// 对于一定处于半径范围内的格子里面的所有对象，可跳过距离判断

	// todo: 填充静态圆形逐步扩散格子, 里面存储 格子xy相对下标
	// 支持距离映射：用半径查 扩散范围. 
	// 假设: 100 米半径，填充了 5000 个扩散格子，则 10 米半径 可能只需要扫前 100 个

	return 0;
}
