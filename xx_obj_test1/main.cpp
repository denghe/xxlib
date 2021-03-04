#include "xx_obj.h"
#include <iostream>

void Test1() {
	xx::ObjManager om;
	auto s = om.ToString("asdf", 123, " ", xx::Now());
	std::cout << s << std::endl;

	// todo: more test here
}

void Test2() {
}

int main() {
	Test1();
	Test2();

	std::cout << "end." << std::endl;
	return 0;
}
