#include "p1.h"
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
	b->data.Fill({ 1,2,3,4,5 });
	b->c.x = 1.2f;
	b->c.y = 3.4f;
	b->c.targets.emplace_back(a);
	b->c.targets.emplace_back(b);
	b->c2 = b->c;
	b->c3.emplace_back().push_back(b->c2);

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
}

int main() {
	Test1();
	Test2();

	std::cout << "end." << std::endl;
	return 0;
}
