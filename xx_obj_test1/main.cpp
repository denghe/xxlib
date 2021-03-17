﻿#include "p1.h"
#include <iostream>

void Test1() {
	CodeGen_shared::Register();
	CodeGen_p1::Register();

	xx::ObjManager om;
	xx::Data d;
	C c;
	c.x = 1.2;
	c.y = 3.4;
	c.targets.emplace_back();
	c.targets.emplace_back();
	c.targets.emplace_back();
	om.WriteTo(d, c);
	std::cout << d.len << std::endl;
	for (size_t i = 0; i < d.len; i++)
	{
		std::cout << (int)d[i] << " ";
	}
	std::cout << std::endl;
}

void Test2() {
}

int main() {
	Test1();
	Test2();

	std::cout << "end." << std::endl;
	return 0;
}
