#include <xx_bufholder.h>
#include <xx_string.h>

int main() {
	struct XY { float x, y; };
	struct XYZ : XY { float z; };
	xx::BufHolder bh;
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
	bh.Get<XY>(2);
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
	bh.Get<XYZ>(2);
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
	bh.Get<XY>(3);
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
	bh.Get<char*>(4);
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
	bh.Shrink();
	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);

	return 0;
}



//#include <xx_string.h>
//
//struct Foo {
//	int Value1;
//	double Value2;
//	double Value3;
//	double Value4;
//	double Value5;
//	double Value6;
//	double Value7;
//	double Value8;
//	double Value9;
//};
////auto counter = 0LL;
////XX_NOINLINE void foo_f(Foo const& foo) {
////    counter += foo.Value1;
////};
//int main() {
//	auto counter = 0LL;
//	auto foo_f = [&](Foo const& foo) {
//		counter += foo.Value1;
//	};
//	Foo foo{ 1 };
//	for (int j = 0; j < 10; ++j) {
//		auto secs = xx::NowEpochSeconds();
//		for (int i = 0; i < 100000000; ++i) {
//			foo_f(foo);
//		}
//		xx::CoutN("foo_f secs = ", xx::NowEpochSeconds(secs), " counter = ", counter);
//	}
//	return 0;
//}


//#include "xx_helpers.h"
//#include "xx_string.h"
//
//struct A {
//    virtual A* xxxx() { return this; };
//};
//struct B : A {
//    B* xxxx() override { return this; };
//};
//
//int main() {
//    B b;
//    xx::CoutN( (size_t)&b );
//    xx::CoutN( (size_t)b.xxxx() );
//
//    return 0;
//}
//
//
////#include "xx_helpers.h"
////#include "xx_string.h"
////int main() {
////    xx::Data d;
////    auto t = xx::NowEpochSeconds();
////    d.Reserve(4000000000);
////    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
////    std::cin.get();
////    t = xx::NowEpochSeconds();
////    for (size_t i = 0; i < 1000000000; i++) {
////        d.WriteFixed((uint32_t)123);
////    }
////    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
////    xx::CoutN("d.len = ", d.len, "d.cap = ", d.cap);
////    std::cin.get();
////    return 0;
////}
