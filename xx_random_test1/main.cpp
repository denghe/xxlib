#include <xx_string.h>

int main() {
    std::cout << xx::UnXor<"qwerqwer">() << std::endl;
    std::cout << xx::UnXor<"qwerqwerqwerqwer">() << std::endl;
    std::cout << xx::UnXor<"asdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdf">() << std::endl;
}




//
//#include <xx_string.h>
//
//template<size_t len>
//struct Str {
//    std::array<char, len - 1> buf;
//    constexpr Str(const char(&str)[len]) {
//        for (size_t i = 0; i < len - 1; ++i) {
//            buf[i] = str[i] ^ 123;
//        }
//    }
//};
//
//template<Str str>
//std::string Decrypt() {
//    std::string rtv(str.buf.data(), str.buf.size());
//    for (auto& c : rtv) c ^= 123;
//    return rtv;
//}
//
//int main() {
//    std::cout << Decrypt<"asdf">() << std::endl;
//}
//





//#include <xx_randoms.h>
//#include <xx_string.h>
//
//int main() {
//	//{
//	//	xx::Random4 rnd;
//	//	for (size_t i = 0; i < 10; i++) {
//	//		xx::CoutN(rnd.Next());
//	//	}
//	//	uint64_t counter = 0;
//	//	auto secs = xx::NowSteadyEpochSeconds();
//	//	for (size_t i = 0; i < 100000000; i++) {
//	//		counter += rnd.Next();
//	//	}
//	//	xx::CoutN("Random4 counter = ", counter, " secs = ", xx::NowSteadyEpochSeconds() - secs);
//	//}
//
//	//{
//	//	xx::Random5 rnd;
//	//	for (size_t i = 0; i < 10; i++) {
//	//		xx::CoutN(rnd.Next());
//	//	}
//	//	uint64_t counter = 0;
//	//	auto secs = xx::NowSteadyEpochSeconds();
//	//	for (size_t i = 0; i < 100000000; i++) {
//	//		counter += rnd.Next();
//	//	}
//	//	xx::CoutN("Random5 counter = ", counter, " secs = ", xx::NowSteadyEpochSeconds() - secs);
//	//}
//
//	{
//		xx::Random5 rnd;
//		xx::Data d;
//		d.Write(rnd);
//		for (size_t i = 0; i < 10; i++) {
//			xx::CoutN(rnd.Next());
//		}
//		xx::CoutN();
//		d.Read(rnd);
//		for (size_t i = 0; i < 10; i++) {
//			xx::CoutN(rnd.Next());
//		}
//	}
//
//	return 0;
//}
