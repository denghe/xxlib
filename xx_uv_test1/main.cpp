#ifdef _WIN32
#pragma comment(lib, "libuv.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "userenv.lib")
#endif
#include <iostream>
#include "xx_uv_ext2.h"

int main() {

	std::cout << "end." << std::endl;
	return 0;
}
