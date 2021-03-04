#include "xx_data.h"
#include <chrono>
#include <iostream>
#include <string>

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
	assert(d.len == 0);
	assert(d.offset == 0);
	assert(d.cap == 0);

	d.Fill({ 1, 2, 3, 4 });
	assert(d.buf != nullptr);
	assert(d.len == 4);
	assert(d.offset == 0);
	assert(d.cap > 3);
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

		uint64_t v = 0;
		int r = dv.ReadFixedBE(v);
		assert(r == 0);
		assert(v == 0x0102030401020304u);
		assert(dv.offset == 8);
	}

	d.WriteFixedBEAt(0, (uint64_t)0);
	assert(d.len == 12);
	assert(d[0] == 0);
	assert(d[1] == 0);
	assert(d[2] == 0);
	assert(d[3] == 0);
	assert(d[4] == 0);
	assert(d[5] == 0);
	assert(d[6] == 0);
	assert(d[7] == 0);
	d.WriteFixedBEAt(0, (uint64_t)0x0102030405060708u);
	assert(d[0] == 1);
	assert(d[1] == 2);
	assert(d[2] == 3);
	assert(d[3] == 4);
	assert(d[4] == 5);
	assert(d[5] == 6);
	assert(d[6] == 7);
	assert(d[7] == 8);
	d.WriteFixedBEAt(0, (uint64_t)0);
	d.WriteFixedBEAt(0, (uint16_t)0x0102u);
	assert(d[0] == 1);
	assert(d[1] == 2);


	auto d2 = d;
	assert(d2 == d);
	assert(d2.buf != d.buf);
	d2.Clear(true);
	assert(d2.buf == nullptr);
	assert(d2.len == 0);
	assert(d2.offset == 0);
	assert(d2.cap == 0);
	d2.Reset(d.buf, d.len, 4, 16);
	assert(d2.buf == d.buf);
	assert(d2.len == d.len);
	assert(d2.offset == 4);
	assert(d2.cap == 16);
	d2.Reset();
	assert(d2.buf == nullptr);
	assert(d2.len == 0);
	assert(d2.offset == 0);
	assert(d2.cap == 0);

	std::string s = "asdf";
	d2.WriteVarInteger(s.size());		// uint[64]
	assert(d2.buf != nullptr);
	assert(d2.len == 1);
	assert(d2.offset == 0);
	assert(d2.cap > d2.len);
	d2.WriteBuf(s.data(), s.size());
	assert(d2.buf != nullptr);
	assert(d2.len == 5);
	assert(d2.offset == 0);
	assert(d2.cap > d2.len);
	assert(d2[0] == 4);
	assert(d2[1] == 'a');
	assert(d2[2] == 's');
	assert(d2[3] == 'd');
	assert(d2[4] == 'f');
	d2.Clear();
	assert(d2.buf != nullptr);
	assert(d2.len == 0);
	assert(d2.offset == 0);
	assert(d2.cap > d2.len);
	d2.WriteVarInteger(-1);
	assert(d2.buf != nullptr);
	assert(d2.len == 1);
	assert(d2.offset == 0);
	assert(d2.cap > d2.len);
	assert(d2[0] == 1);
	d2.Clear();
	d2.WriteVarInteger(1);
	assert(d2[0] == 2);
	d2.Clear();
	d2.WriteVarInteger(2);
	assert(d2[0] == 4);
	d2.Clear();
	d2.WriteVarInteger(-2);
	assert(d2[0] == 3);
	d2.Clear();
	d2.WriteVarInteger((uint32_t)127);
	assert(d2.len == 1);
	assert(d2[0] == 127);
	d2.Clear();
	d2.WriteVarInteger((uint32_t)128);
	assert(d2.len == 2);
	assert(d2[0] == 128);
	assert(d2[1] == 1);
	d2.Clear();
	d2.WriteVarInteger((uint32_t)255);
	assert(d2.len == 2);
	assert(d2[0] == 255);
	assert(d2[1] == 1);
	d2.Clear();
	d2.WriteVarInteger((uint64_t)0xFFFFFFFFFFFFFFFF);
	assert(d2.len == 10);
	assert(d2[0] == 255);
	assert(d2[1] == 255);
	assert(d2[2] == 255);
	assert(d2[3] == 255);
	assert(d2[4] == 255);
	assert(d2[5] == 255);
	assert(d2[6] == 255);
	assert(d2[7] == 255);
	assert(d2[8] == 255);
	assert(d2[9] == 1);
	d2.Clear();
	d2.WriteVarInteger((uint64_t)0x7FFFFFFFFFFFFFFF);
	assert(d2.len == 9);
	assert(d2[0] == 255);
	assert(d2[1] == 255);
	assert(d2[2] == 255);
	assert(d2[3] == 255);
	assert(d2[4] == 255);
	assert(d2[5] == 255);
	assert(d2[6] == 255);
	assert(d2[7] == 255);
	assert(d2[8] == 127);

	std::cout << "end." << std::endl;
	return 0;
}
