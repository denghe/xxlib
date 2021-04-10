#include "p1.h"
#include "foo.h"
#include <iostream>

// om 增加快速读写模式, 不处理递归引用( 生成阶段可根据类结构判断 是否含有互引用 ). 即便可能递归的结构，也可以手工强制使用( 可 assert recursive check )
// 有无递归可能，使用 type 来标注, has type 来检测??
// 初期可先手工强制 + assert，发送方自己应该清楚是否含有递归, 接收则为不信任原则

// 针对 发送的类 无派生类 的情况，可 check type 标注（ 生成器辅助生成 ), 然后调用 o->T::RW 能跳过虚函数跳表

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
    b->data.Fill({1, 2, 3, 4, 5});
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
    auto ha = ((xx::PtrHeader *) a.pointer - 1);
    auto he = ((xx::PtrHeader *) e.pointer - 1);
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
    a->data.Fill({1, 2, 3, 4, 5});
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

void Test3() {
    xx::ObjManager om;
    xx::Data d;
    d.Reserve(100);

    auto f = xx::MakeShared<foo>();
    f->id = 100;
    f->name = "111111";

    auto b = xx::MakeShared<bar>();
    b->id = 100;
    b->name = "111111";

    foo2 f2;
    f2.id = 100;
    f2.name = "111111";


    auto fb = xx::MakeShared<FishBase>();

    auto fwc = xx::MakeShared<FishWithChilds>();
    fwc->childs.push_back(fwc);
    auto sg_fwc = xx::MakeScopeGuard([&] { om.KillRecursive(fwc); });


    for (int j = 0; j < 10; ++j) {
        {
            auto s = xx::NowEpochSeconds();
            for (int i = 0; i < 10000000; ++i) {
                d.Clear();
                om.WriteTo(d, f);
            }
            xx::CoutN("om.WriteTo(d, f)            ", xx::NowEpochSeconds() - s, d);
        }
        {
            auto s = xx::NowEpochSeconds();
            for (int i = 0; i < 10000000; ++i) {
                d.Clear();
                om.WriteTo<true>(d, f);
            }
            xx::CoutN("om.WriteTo<true>(d, f)      ", xx::NowEpochSeconds() - s, d);
        }
        {
            auto s = xx::NowEpochSeconds();
            for (int i = 0; i < 10000000; ++i) {
                d.offset = 0;
                om.ReadFrom(d, f);
            }
            xx::CoutN("om.ReadFrom(d, f)      ", xx::NowEpochSeconds() - s, d);
        }
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

    xx::CoutN(om.ToString(f));
}

int main() {
    //Test1();
    //Test2();
    Test3();

    std::cout << "end." << std::endl;
    return 0;
}
