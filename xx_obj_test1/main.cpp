#include "p1.h"
#include <iostream>

void Test1() {
	CodeGen_shared::Register();
	CodeGen_p1::Register();


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
	std::cout << om.ToString(b) << std::endl;

	xx::Data d;
	om.WriteTo(d, b);
	std::cout << om.ToString(d) << std::endl;

	xx::ObjBase_s c;
	int r = om.ReadFrom(d, c);
	assert(!r);
	std::cout << om.ToString(c) << std::endl;

	//om.KillRecursive()
}

void Test2() {
}

int main() {
	Test1();
	Test2();

	std::cout << "end." << std::endl;
	return 0;
}
