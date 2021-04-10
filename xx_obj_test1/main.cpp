﻿#include "p1.h"
#include "foo.h"
#include <iostream>

// todo: 针对 “简单类型”， 似乎可以批量 Reserve。生成的代码可提前来一发，中途的写入就跳过

void Test1() {
	auto a = xx::MakeShared<A>();
	a->id = 11;
	a->nick = "aaa";
	a->parent = a;
	a->children.push_back(a);

	auto b = xx::MakeShared<B>();
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
	std::cout << om.ToString(a) << std::endl;
	std::cout << om.ToString(b) << std::endl;

	xx::Data d;
	om.WriteTo(d, b);
	std::cout << om.ToString(d) << std::endl;

	xx::ObjBase_s c;
	int r = om.ReadFrom(d, c);
	assert(!r);
	std::cout << om.ToString(c) << std::endl;
	assert(om.ToString(b) == om.ToString(c));

	std::cout << "auto e = om.Clone(a);" << std::endl;
	auto e = om.Clone(a);
	auto ha = ((xx::PtrHeader*)a.pointer - 1);
	auto he = ((xx::PtrHeader*)e.pointer - 1);
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
	auto a = xx::MakeShared<B>();
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
	//auto f = xx::MakeShared<foo>();
	//f->id = 100;
	//f->name = "111111";

	//auto b = xx::MakeShared<bar>();
	//b->id = 100;
	//b->name = "111111";

	foo2 f2;
	f2.id = 100;
	f2.name = "111111";


	//auto fb = xx::MakeShared<FishBase>();

	//auto fwc = xx::MakeShared<FishWithChilds>();
	//fwc->childs.push_back(fwc);
	//auto sg_fwc = xx::MakeScopeGuard([&] { om.KillRecursive(fwc); });

	d.Reserve(10000000 * 5 * 3);

	for (int j = 0; j < 10; ++j) {
		{
			d.Clear();
			auto s = xx::NowEpochSeconds();
			for (int i = 0; i < 10000000; ++i) {
				om.Write(d, f2);
			}
			xx::CoutN("om.Write(d, f2);      ", xx::NowEpochSeconds() - s, " ", d.len);
		}
		//{
		//	auto s = xx::NowEpochSeconds();
		//	for (int i = 0; i < 10000000; ++i) {
		//		int n;
		//		d.ReadVarInteger(n);
		//	}
		//	xx::CoutN("d.ReadVarInteger(n);              ", xx::NowEpochSeconds() - s, " ", d.offset);
		//}
		//uint64_t crc = 0;
		//for (size_t i = 0; i < d.len; ++i) {
		//	crc += (uint64_t)d[i];
		//}
		//xx::CoutN("d.crc = ", crc);
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo(d, b);
		//            }
		//            xx::CoutN("om.WriteTo(d, b)            ", xx::NowEpochSeconds() - s, d);
		//        }
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo<true>(d, b);
		//            }
		//            xx::CoutN("om.WriteTo<true>(d, b)      ", xx::NowEpochSeconds() - s, d);
		//        }
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo(d, f2);
		//            }
		//            xx::CoutN("om.WriteTo(d, f2)           ", xx::NowEpochSeconds() - s, d);
		//        }
		//
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo(d, fb);
		//            }
		//            xx::CoutN("om.WriteTo(d, fb)           ", xx::NowEpochSeconds() - s, d);
		//        }
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo<true>(d, fb);
		//            }
		//            xx::CoutN("om.WriteTo<true>(d, fb)     ", xx::NowEpochSeconds() - s, d);
		//        }
		//
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo(d, fwc);
		//            }
		//            xx::CoutN("om.WriteTo(d, fwc)          ", xx::NowEpochSeconds() - s, d);
		//        }
		//        {
		//            auto s = xx::NowEpochSeconds();
		//            for (int i = 0; i < 10000000; ++i) {
		//                d.Clear();
		//                om.WriteTo<true>(d, fwc);
		//            }
		//            xx::CoutN("om.WriteTo<true>(d, fwc)    ", xx::NowEpochSeconds() - s, d);
		//        }


				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        om.Write(d, f2);
				//    }
				//    xx::CoutN("om.Write(d, f2)             ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        om.Write(d, *f);
				//    }
				//    xx::CoutN("om.Write(d, *f)             ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        f->Write(om, d);
				//    }
				//    xx::CoutN("f->Write(om, d)             ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        f->foo::Write(om, d);
				//    }
				//    xx::CoutN("f->foo::Write(om, d)        ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        b->Write(om, d);
				//    }
				//    xx::CoutN("b->Write(om, d)             ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        b->bar::Write(om, d);
				//    }
				//    xx::CoutN("b->bar::Write(om, d)        ", xx::NowEpochSeconds() - s, d);
				//}
				//{
				//    auto s = xx::NowEpochSeconds();
				//    for (int i = 0; i < 10000000; ++i) {
				//        d.Clear();
				//        om.Write(d, f->id, f->name);
				//    }
				//    xx::CoutN("om.Write(d, f->id, f->name) ", xx::NowEpochSeconds() - s, d);
				//}
	}

	//xx::CoutN(om.ToString(f));
}

int main() {
	//Test1();
	//Test2();
	Test3();

	std::cout << "end." << std::endl;
	return 0;
}
