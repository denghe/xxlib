#include "xx_helpers.h"
#include "xx_string.h"

struct A {
    virtual A* xxxx() { return this; };
};
struct B : A {
    B* xxxx() override { return this; };
};

int main() {
    B b;
    xx::CoutN( (size_t)&b );
    xx::CoutN( (size_t)b.xxxx() );

    return 0;
}


//#include "xx_helpers.h"
//#include "xx_string.h"
//int main() {
//    xx::Data d;
//    auto t = xx::NowEpochSeconds();
//    d.Reserve(4000000000);
//    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
//    std::cin.get();
//    t = xx::NowEpochSeconds();
//    for (size_t i = 0; i < 1000000000; i++) {
//        d.WriteFixed((uint32_t)123);
//    }
//    xx::CoutN("elapsed secs = ", xx::NowEpochSeconds() - t);
//    xx::CoutN("d.len = ", d.len, "d.cap = ", d.cap);
//    std::cin.get();
//    return 0;
//}
