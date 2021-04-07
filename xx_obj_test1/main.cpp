#include "p1.h"
#include "foo.h"
#include <iostream>

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
    d.Reserve(100000000);
    auto f = xx::MakeShared<foo>();
    f->id = 100;
    f->name = "111111";
    auto s = xx::NowEpochSeconds();
    for (int i = 0; i < 10000000; ++i) {
        d.WriteFixed(i);
        //om.WriteTo(d, f);
    }
    xx::CoutN(xx::NowEpochSeconds() - s);
    std::cin.get();
    xx::CoutN(d);
}

int main() {
    //Test1();
    //Test2();
    Test3();

    std::cout << "end." << std::endl;
    return 0;
}
