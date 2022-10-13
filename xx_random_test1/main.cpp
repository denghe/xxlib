#include <xx_string_http.h>

namespace xx {

	template<Str s>
	std::string HtmlPreEncode1() {
		std::string r;
		r.reserve(sizeof(s.v) * 5);
		for (auto& c : s.v) {
			if (c == '&') r.append("&amp;");
			else if (c == '<') r.append("&lt;");
			else if (c == '>') r.append("&gt;");
			else r.push_back(c);
		}
		return r;
	}

	std::string HtmlPreEncode2(std::string_view const& s) {
		std::string r;
		r.reserve(s.size() * 5);
		for (auto& c : s) {
			if (c == '&') r.append("&amp;");
			else if (c == '<') r.append("&lt;");
			else if (c == '>') r.append("&gt;");
			else r.push_back(c);
		}
		return r;
	}

	template<char c>
	size_t HtmlPreEncode3___() {
		if (c == '&') return 5;
		else if (c == '<' || c == '>') return 4;
		else return 1;
	}
	template<char c>
	void HtmlPreEncode3__(std::string& r) {
		if (c == '&') r.append("&amp;");
		else if (c == '<') r.append("&lt;");
		else if (c == '>') r.append("&gt;");
		else r.push_back(c);
	}
	template<Str s, size_t... I>
	void HtmlPreEncode3_(std::string& r, std::index_sequence<I...>) {
		r.reserve((HtmlPreEncode3___<s.v[I]>() + ...));
		(HtmlPreEncode3__<s.v[I]>(r), ...);
	}
	template<Str s>
	std::string HtmlPreEncode3() {
		std::string r;
		HtmlPreEncode3_<s>(r, std::make_index_sequence<sizeof(s.v) - 1>());
		return r;
	}
}

int main() {
	std::string s;
	for (size_t j = 0; j < 10; j++) {
		auto secs = xx::NowEpochSeconds();
		for (size_t i = 0; i < 10000000; i++) {
			s = xx::HtmlPreEncode1<"as<>&dfqw<>&eras<>&dfqw<>&er ">();
		}
		std::cout << "1 " << s << xx::NowEpochSeconds(secs) << std::endl;

		for (size_t i = 0; i < 10000000; i++) {
			s = xx::HtmlPreEncode2("as<>&dfqw<>&eras<>&dfqw<>&er "sv);
		}
		std::cout << "2 " << s << xx::NowEpochSeconds(secs) << std::endl;

		for (size_t i = 0; i < 10000000; i++) {
			s = xx::HtmlPreEncode3<"as<>&dfqw<>&eras<>&dfqw<>&er ">();
		}
		std::cout << "3 " << s << xx::NowEpochSeconds(secs) << std::endl;
		std::cout << std::endl;
	}
}




//#include <xx_string.h>
//
//int main() {
//    std::cout << xx::UnXor<"qwerqwer">() << std::endl;
//    std::cout << xx::UnXor<"qwerqwerqwerqwer">() << std::endl;
//    std::cout << xx::UnXor<"asdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdfasdf">() << std::endl;
//}
//



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
