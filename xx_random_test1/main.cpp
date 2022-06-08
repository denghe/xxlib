#include <xx_randoms.h>
#include <xx_string.h>

int main() {
	//{
	//	xx::Random4 rnd;
	//	for (size_t i = 0; i < 10; i++) {
	//		xx::CoutN(rnd.Next());
	//	}
	//	uint64_t counter = 0;
	//	auto secs = xx::NowSteadyEpochSeconds();
	//	for (size_t i = 0; i < 100000000; i++) {
	//		counter += rnd.Next();
	//	}
	//	xx::CoutN("Random4 counter = ", counter, " secs = ", xx::NowSteadyEpochSeconds() - secs);
	//}

	//{
	//	xx::Random5 rnd;
	//	for (size_t i = 0; i < 10; i++) {
	//		xx::CoutN(rnd.Next());
	//	}
	//	uint64_t counter = 0;
	//	auto secs = xx::NowSteadyEpochSeconds();
	//	for (size_t i = 0; i < 100000000; i++) {
	//		counter += rnd.Next();
	//	}
	//	xx::CoutN("Random5 counter = ", counter, " secs = ", xx::NowSteadyEpochSeconds() - secs);
	//}

	{
		xx::Random5 rnd;
		xx::Data d;
		d.Write(rnd);
		for (size_t i = 0; i < 10; i++) {
			xx::CoutN(rnd.Next());
		}
		xx::CoutN();
		d.Read(rnd);
		for (size_t i = 0; i < 10; i++) {
			xx::CoutN(rnd.Next());
		}
	}

	return 0;
}
