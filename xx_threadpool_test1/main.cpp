#include "xx_threadpool.h"
#include <iostream>

int main() {
	xx::ThreadPool<> tp;
	for (size_t i = 0; i < 100; i++)
	{
		tp.Add([] {std::cout << std::this_thread::get_id() << std::endl; });
	}
	std::cout << "end." << std::endl;
	return 0;
}
