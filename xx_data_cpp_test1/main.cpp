#include "xx_data.h"
#include <chrono>
#include <iostream>

void Test1() {
	xx::Data d;
	d.Reserve(1024 * 1024 * 1024);
	for (int j = 0; j < 10; ++j) {
		auto t = std::chrono::system_clock::now();
		d.Clear();
		for (uint64_t i = 0; i < 100000000; ++i) {
			d.WriteFixed(i);
		}
		std::cout << std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() - t)).count() << std::endl;
		std::cout << d.len << std::endl;
	}
}

int main() {
	//Test1();

	xx::Data d;
	assert(d.buf == nullptr);
	assert(d.cap == 0);
	assert(d.len == 0);
	assert(d.offset == 0);

	d.Fill({ 1, 2, 3, 4 });
	assert(d.buf != nullptr);
	assert(d.cap > 3);
	assert(d.len == 4);
	assert(d.offset == 0);
	d.WriteFixed(0x04030201u);
	assert(d.len == 8);
	assert(d[0] == d[4]);
	assert(d[1] == d[5]);
	assert(d[2] == d[6]);
	assert(d[3] == d[7]);
	d.WriteFixedBE(0x01020304u);
	assert(d.len == 12);
	assert(d[0] == d[8]);
	assert(d[1] == d[9]);
	assert(d[2] == d[10]);
	assert(d[3] == d[11]);
	auto d2 = d;
	assert(d2 == d);
	assert(d2.buf != d.buf);

	{
		xx::Data_v dv(d.buf, d.len, 0);
		assert(dv.buf == d.buf);
		assert(dv.len == d.len);
		assert(dv.offset == 0);
		dv.Reset(d);
		assert(dv.buf == d.buf);
		assert(dv.len == d.len);
		assert(dv.offset == 0);
		dv = d;
		assert(dv.buf == d.buf);
		assert(dv.len == d.len);
		assert(dv.offset == 0);

		uint32_t v = 0;
		int r = dv.ReadFixed(v);
		assert(r == 0);
		assert(v == 0x04030201u);
		assert(dv.offset == 4);
	}

	{
		xx::Data_v dv(d);
		assert(dv.buf == d.buf);
		assert(dv.len == d.len);
		assert(dv.offset == 0);

		uint32_t v = 0;
		int r = dv.ReadFixedBE(v);
		assert(r == 0);
		assert(v == 0x01020304u);
		assert(dv.offset == 4);
	}

	std::cout << "end." << std::endl;
	return 0;
}
