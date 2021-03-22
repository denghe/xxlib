#include "xx_queue.h"
#include <iostream>

int main() {
	xx::Queue<int> q;
	q.Push(1, 2, 3);
	std::cout << q.Count() << std::endl;
	std::cout << q[0] << std::endl;
	std::cout << q[1] << std::endl;
	std::cout << q[2] << std::endl;
	q.PopMulti(2);
	std::cout << q.Count() << std::endl;

	std::cout << "end." << std::endl;
	return 0;
}
