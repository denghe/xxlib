#include "p1.h"
#include "foo.h"
#include <iostream>

void Test1() {
	auto a = xx::Make<A>();
	a->id = 11;
	a->nick = "aaa";
	a->parent = a;
	a->children.push_back(a);

	auto b = xx::Make<B>();
	b->id = 22;
	b->nick = "bbbbb";
	b->parent = a;
	b->children.push_back(a);
	b->data.Fill({ 1, 2, 3, 4, 5 });
	b->c.x = 1.2f;
	b->c.y = 3.4f;
	b->c.targets.emplace_back(a);
	b->c.targets.emplace_back(b);
	b->c2 = b->c;

	xx::ObjManager om;
	om.CoutN(a);
	om.CoutN(b);

	xx::Data d;
	om.WriteTo(d, b);
    om.CoutN(d);

	xx::ObjBase_s c;
	int r = om.ReadFrom(d, c);
	assert(!r);
	std::cout << om.ToString(c) << std::endl;
	assert(om.ToString(b) == om.ToString(c));

	std::cout << "auto e = om.Clone(a);" << std::endl;
	auto e = om.Clone(a);
	auto ha = ((xx::ObjPtrHeader*)a.pointer - 1);
	auto he = ((xx::ObjPtrHeader*)e.pointer - 1);
	std::cout << om.ToString(a) << std::endl;
	std::cout << om.ToString(e) << std::endl;
	assert(om.ToString(a) == om.ToString(e));

	std::cout << "auto f = om.Clone(b);" << std::endl;
	auto f = om.Clone(b);
	auto hb = b.header();
	auto hf = f.header();
	std::cout << om.ToString(b) << std::endl;
	std::cout << om.ToString(f) << std::endl;
	assert(om.ToString(b) == om.ToString(f));

	std::cout << "om.HasRecursive(a) = " << om.HasRecursive(a) << std::endl;
	std::cout << "om.HasRecursive(b) = " << om.HasRecursive(b) << std::endl;
	std::cout << "om.HasRecursive(c) = " << om.HasRecursive(c) << std::endl;
	std::cout << "om.HasRecursive(d) = " << om.HasRecursive(d) << std::endl;
	std::cout << "om.HasRecursive(e) = " << om.HasRecursive(e) << std::endl;
	std::cout << "om.HasRecursive(f) = " << om.HasRecursive(f) << std::endl;

	om.KillRecursive(a);
	om.KillRecursive(b);
	om.KillRecursive(c);
	om.KillRecursive(d);
	om.KillRecursive(e);
	om.KillRecursive(f);
}

void Test2() {
	auto a = xx::Make<B>();
	a->id = 1;
	a->nick = "asdf";
	a->parent = a;
	a->children.push_back(a);
	a->data.Fill({ 1, 2, 3, 4, 5 });
	a->c.x = 1.2f;
	a->c.y = 2.3f;
	a->c.targets.push_back(a);
	a->c2 = a->c;
	a->c3.push_back(a->c);

	xx::ObjManager om;
	xx::Data d;
	om.WriteTo(d, a);
	std::cout << om.ToString(d) << std::endl;

	// 2,14,0,0,0,2,1,4,97,115,100,102,1,1,1,5,1,2,3,4,5,154,153,153,63,51,51,19,64,1,1,1,154,153,153,63,51,51,19,64,1,1,1,1,154,153,153,63,51,51,19,64,1,1
	//[2,14,0,0,0,2,1,4,97,115,100,102,1,1,1,5,1,2,3,4,5,154,153,153,63,51,51,19,64,1,1,1,154,153,153,63,51,51,19,64,1,1,1,1,154,153,153,63,51,51,19,64,1,1]
}

#include "xx_string.h"
#include "xx_data_funcs.h"

void Test3() {
	xx::ObjManager om;
	xx::Data d;

	//d.Reserve(100);
	// 
	//auto f = xx::Make<foo>();
	//f->id = 100;
	//f->name = "111111";

	//auto b = xx::Make<bar>();
	//b->id = 100;
	//b->name = "111111";

	foo2 f2;
	f2.id = 100;
	f2.d.Fill({ 1,1,1,1,1,1 });
	om.CoutN(f2);

	d.Reserve(10000000 * 5 * 3);

	for (int j = 0; j < 10; ++j) {
		{
			d.Clear();
			auto s = xx::NowEpochSeconds();
			for (int i = 0; i < 10000000; ++i) {
				om.WriteTo(d, f2);
			}
			xx::CoutN("om.WriteTo(d, f2);             secs = ", xx::NowEpochSeconds() - s, " d.len = ", d.len, " d.cap = ", d.cap);
		}
		{
			d.Clear();
			auto s = xx::NowEpochSeconds();
			for (int i = 0; i < 10000000; ++i) {
				om.WriteTo<false>(d, f2);
			}
			xx::CoutN("om.WriteTo<false>(d, f2);      secs = ", xx::NowEpochSeconds() - s, " d.len = ", d.len, " d.cap = ", d.cap);
		}
		{
			auto s = xx::NowEpochSeconds();
			//int id;
			//xx::Data dd;
			//foo2 f3;
			for (int i = 0; i < 10000000; ++i) {
				if (int r = om.ReadFrom(d, f2)) break;
				//if (int r = om.Read(d, f2)) break;
				//if (int r = d.Read(f2.id, f2.d)) break;
				//if (int r = xx::ObjFuncs<foo2>::Read(om, d, f2)) break;
				//if (int r = d.Read(id, dd)) break;

			}
			xx::CoutN("om.ReadFrom(d, f2)             secs = ", xx::NowEpochSeconds() - s, " d.len = ", d.len, " d.cap = ", d.cap, " d.offset = ", d.offset);
		}
		xx::CoutN(om.ToString(f2));
	}
}

void Test4() {
	//std::variant<int64_t, std::string> v1, v2;
	//v1 = 123;
	//v2 = "asdf";
	//xx::Data d;
	//d.Write(v1, v2);
	//xx::CoutN(d);
	//xx::CoutN(v1);
	//xx::CoutN(v2);
	//if (int r = d.Read(v2)) throw r;
	//if (int r = d.Read(v1)) throw r;
	//xx::CoutN(v1);
	//xx::CoutN(v2);
	std::map<std::variant<int64_t, std::string>, std::variant<int64_t, std::string>> vs, vs2;
	vs.emplace("a", 123);
	vs.emplace(22, "asdf");
	xx::Data d;
	d.Write(vs);
	xx::CoutN(d);
	if (int r = d.Read(vs2)) throw r;
	xx::CoutN(vs2);
}

void Test5() {
	struct VVV : std::map<
		std::variant<int64_t, std::string, std::shared_ptr<VVV>>, 
		std::variant<int64_t, std::string, std::shared_ptr<VVV>>> {};
	auto v = std::make_shared<VVV>();
	v->emplace(v, v);
}

int main() {
	//Test1();
	//Test2();
	Test3();
	//Test4();
	//Test5();

	std::cout << "end." << std::endl;
	return 0;
}








///*
//🀀🀁🀂🀃🀆🀅🀄️
//🀇🀈🀉🀊🀋🀌🀍🀎🀏
//🀐🀑🀒🀓🀔🀕🀖🀗🀘
//🀙🀚🀛🀜🀝🀞🀟🀠🀡
//
//🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮
//🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾
//🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎
//🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞
//🃟🃏
//*/
//
//int main() {
//    // windows console is not support
//    auto c = u8R"(
//🀀🀁🀂🀃🀆🀅🀄️
//🀇🀈🀉🀊🀋🀌🀍🀎🀏
//🀐🀑🀒🀓🀔🀕🀖🀗🀘
//🀙🀚🀛🀜🀝🀞🀟🀠🀡
//
//🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮
//🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾
//🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎
//🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞
//🃟🃏
//)";
//    std::cout << (char*)c << std::endl;
//
//	//Test1();
//	//Test2();
//	//Test3();
//
//	std::cout << "end." << std::endl;
//	return 0;
//}
