#include "xx_dict.h"
#include <iostream>

int main() {
	xx::Dict<int, std::string> d;
	d.Add(1, "asdf");
	std::cout << d[1] << std::endl;

	return 0;
}
