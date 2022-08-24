#include <iostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

using var_t = std::variant<double, std::string>;

template<typename...Ts> struct overloaded : Ts...{ using Ts::operator()...; };

int main() {
    std::vector<var_t> vars = { 1.2, "asdf" };
    for (auto&& v : vars) {
        std::visit(overloaded{ 
            [](auto a) { std::cout << "auto a = " << a << std::endl; },
            [](double a) { std::cout << "double a = " << a << std::endl; },
            [](std::string const& a) { std::cout << "string a = " << a << std::endl; },
        }, v);
    }
    return 0;
}


//#include <xx_string.h>
//
//int main() {
//    {
//        std::string s;
//        xx::AppendFormat(s, "{0} {1}", 1.234, "asdf"sv);
//        std::cout << s << std::endl;
//    }
//
//    {
//        auto str = R"(
//
// 1, 2,3
//4,5     ,6
//
//7,       8,9 )"sv;
//
//        auto s = str;
//        while (!s.empty()) {
//            auto line = xx::SplitOnce(s, "\n");
//            line = xx::Trim(line);
//            if (line.empty()) continue;
//            while (!line.empty()) {
//                auto word = xx::SplitOnce(line, ",");
//                word = xx::Trim(word);
//                int n;
//                if (xx::SvToNumber(word, n)) {
//                    std::cout << n << std::endl;
//                }
//            }
//        }
//
//        s = str;
//        while (!s.empty()) {
//            if (auto word = xx::Trim(xx::SplitOnce(s, "\n,")); !word.empty()) {
//                std::cout << xx::SvToNumber<int>(word) << std::endl;
//            }
//        }
//    }
//
//    return 0;
//}


//#include <xx_dict.h>
//#include <xx_string.h>
//#include <unordered_map>
//#include <tsl/hopscotch_map.h>
//
//int main() {
//
//	xx::Dict<int, int> d1;
//	std::unordered_map<int, int> d2;
//	tsl::hopscotch_map<int, int, xx::Hash<int>> d3;
//	int n = 10000000;
//
//	for (int j = 0; j < 3; j++) {
//		{
//			d1.Clear();
//			auto secs = xx::NowSteadyEpochSeconds();
//			for (int i = 0; i < n; ++i) {
//				d1.Add(i, i);
//			}
//			xx::CoutN("insert ", n, " rows xx::Dict secs = ", xx::NowSteadyEpochSeconds() - secs);
//		}
//		{
//			d2.clear();
//			auto secs = xx::NowSteadyEpochSeconds();
//			for (int i = 0; i < n; ++i) {
//				d2.emplace(i, i);
//			}
//			xx::CoutN("insert ", n, " rows std::unordered_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//		}
//		{
//			d3.clear();
//			auto secs = xx::NowSteadyEpochSeconds();
//			for (int i = 0; i < n; ++i) {
//				d3.emplace(i, i);
//			}
//			xx::CoutN("insert ", n, " rows tsl::hopscotch_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//		}
//	}
//
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d1.Find(j); r != -1) {
//				counter += d1.ValueAt(r);
//			}
//		}
//		xx::CoutN("counter = ", counter, ", xx::Dict secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d2.find(j); r != d2.end()) {
//				counter += r->second;
//			}
//		}
//		xx::CoutN("counter = ", counter, ", std::unordered_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d3.find(j); r != d3.end()) {
//				counter += r->second;
//			}
//		}
//		xx::CoutN("counter = ", counter, ", tsl::hopscotch_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//}








//#include <xx_dict.h>
//#include <xx_string.h>
//#include <unordered_map>
//#include <tsl/hopscotch_map.h>
//
//#include <xxh3.h>
//namespace xx {
//	// 适配 std::string
//	template<typename T>
//	struct Hash<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view>>> {
//		inline size_t operator()(T const& k) const {
//			return (size_t)XXH3_64bits(k.data(), k.size());
//		}
//	};
//}
//struct string_hash {
//	using hash_type = xx::Hash<std::string_view>; 
//	//using hash_type = std::hash<std::string_view>;
//	using is_transparent = void;
//	size_t operator()(const char* str) const { return hash_type{}(str); }
//	size_t operator()(std::string_view str) const { return hash_type{}(str); }
//	size_t operator()(std::string const& str) const { return hash_type{}(str); }
//};
//int main() {
//    // 测试结论：5950x win msvc Dict 性能大约是 后者的 2-3 倍, m1 macos clang Dict 比后者略慢
//
//	xx::Dict<std::string_view, int> d1;
//	std::unordered_map<std::string_view, int, string_hash, std::equal_to<void>> d2;
//	tsl::hopscotch_map<std::string_view, int, string_hash, std::equal_to<void>> d3;
//	int n = 1000000;
//	std::vector<std::string> ss;
//	for (int j = 0; j < n; j++) {
//		ss.emplace_back(std::to_string(j) + "asdfqwerasdf"
//			//"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//			//"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//			//"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//			//"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//			//"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
//		);
//	}
//	std::vector<std::string_view> svs;
//	for (auto& s : ss) {
//		svs.push_back(s);
//	}
//	for (int i = 0; i < n; ++i) {
//		d1.Add(ss[i], i);
//		d2.emplace(ss[i], i);
//		d3.emplace(ss[i], i);
//	}
//
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d1.Find(svs[j]); r != -1) {
//				counter += d1.ValueAt(r);
//			}
//		}
//		xx::CoutN("counter = ", counter, ", xx::Dict secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d2.find(svs[j]); r != d2.end()) {
//				counter += r->second;
//			}
//		}
//		xx::CoutN("counter = ", counter, ", std::unordered_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			if (auto r = d3.find(svs[j]); r != d3.end()) {
//				counter += r->second;
//			}
//		}
//		xx::CoutN("counter = ", counter, ", tsl::hopscotch_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//}







// 一些测试结论:  tsl::hopscotch_map 主要适合用于 int 等小 key. std string 等大 key 性能 似乎还不如 std::unordered_map.  xx::Dict 小key 不如 tsl, 但综合都比 std::umap 快
//
//#include <tsl/hopscotch_map.h>
//#include <tsl/robin_map.h>
//#include <xx_dict.h>
//
//int main() {
//	tsl::hopscotch_map<int, int, xx::Hash<int>> dict1;
//	std::unordered_map<int, int, xx::Hash<int>> dict2;
//	xx::Dict<int, int> dict3;
//
//	tsl::hopscotch_map<std::string, int> dict4;
//	std::unordered_map<std::string, int> dict5;
//	xx::Dict<std::string, int> dict6;
//
//	std::vector<std::string> ss;
//	for (int j = 0; j < n; j++) {
//		ss.emplace_back(std::to_string(j));
//	}
//	for (int i = 0; i < n; ++i) {
//		dict1.emplace(i, i);
//		dict2.emplace(i, i);
//		dict3.Add(i, i);
//		dict4.emplace(ss[i], i);
//		dict5.emplace(ss[i], i);
//		dict6.Add(ss[i], i);
//	}
//
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict1[j];
//		}
//		xx::CoutN("counter = ", counter, ", hopscotch_map<int secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict2[j];
//		}
//		xx::CoutN("counter = ", counter, ", std::unordered_map<int secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict3[j];
//		}
//		xx::CoutN("counter = ", counter, ", Dict<int secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict4[ss[j]];
//		}
//		xx::CoutN("counter = ", counter, ", hopscotch_map<std::string secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict5[ss[j]];
//		}
//		xx::CoutN("counter = ", counter, ", std::unordered_map<std::string secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//	for (int i = 0; i < 5; i++) {
//		auto secs = xx::NowSteadyEpochSeconds();
//		int64_t counter = 0;
//		for (int j = 0; j < n; j++) {
//			counter += dict6[ss[j]];
//		}
//		xx::CoutN("counter = ", counter, ", Dict<std::string secs = ", xx::NowSteadyEpochSeconds() - secs);
//	}
//}


//struct HString : std::string {
//	using std::string::string;
//	size_t hashCode = 0;
//	void FillHashCode();
//};
//namespace std {
//	template <> struct hash<HString> {
//		size_t operator()(HString const& k) const {
//			return k.hashCode;
//		}
//	};
//}
//void HString::FillHashCode() {
//	hashCode = ::std::hash<std::string>()(*this);
//}


//#include <xx_dict.h>
//#include <tsl/hopscotch_map.h>
//#include <tsl/robin_map.h>
//
//#include <xxh3.h>
//namespace xx {
//	// 适配 std::string
//	template<typename T>
//	struct Hash<T, std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view>>> {
//		inline size_t operator()(T const& k) const {
//			return (size_t)XXH3_64bits(k.data(), k.size());
//		}
//	};
//}

//std::unordered_map<std::string, int> dict1;
//xx::Dict<std::string, int> dict2;
//tsl::hopscotch_map<std::string, int, xx::Hash<std::string>> dict3;

//for (int i = 0; i < n; ++i) {
//	auto s = std::to_string(i);
//	dict1.emplace(s, i);
//	dict2.Add(s, i);
//	dict3.emplace(s, i);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for (int j = 0; j < n; j++) {
//		auto s = std::to_string(j);
//		counter += dict1[s];
//	}
//	xx::CoutN("counter = ", counter, ", unordered_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for( int j = 0; j < n; j++) {
//		auto s = std::to_string(j);
//		counter += dict2[s];
//	}
//	xx::CoutN("counter = ", counter, ", Dict secs = ", xx::NowSteadyEpochSeconds() - secs);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for( int j = 0; j < n; j++) {
//		auto s = std::to_string(j);
//		counter += dict3[s];
//	}
//	xx::CoutN("counter = ", counter, ", hopscotch_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//}



//std::map<int, int> dict1;
//xx::Dict<int, int> dict2;
//tsl::hopscotch_map<int, int, xx::Hash<int>> dict3;

//for (int i = 0; i < n; ++i) {
//	dict1.emplace(i, i);
//	dict2.Add(i, i);
//	dict3.emplace(i, i);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for (int j = 0; j < n; j++) {
//		counter += dict1[j];
//	}
//	xx::CoutN("counter = ", counter, ", map secs = ", xx::NowSteadyEpochSeconds() - secs);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for (int j = 0; j < n; j++) {
//		counter += dict2[j];
//	}
//	xx::CoutN("counter = ", counter, ", Dict secs = ", xx::NowSteadyEpochSeconds() - secs);
//}

//for (int i = 0; i < 10; i++) {
//	auto secs = xx::NowSteadyEpochSeconds();
//	int64_t counter = 0;
//	for (int j = 0; j < n; j++) {
//		counter += dict3[j];
//	}
//	xx::CoutN("counter = ", counter, ", hopscotch_map secs = ", xx::NowSteadyEpochSeconds() - secs);
//}







//#include <xx_nodepool.h>
//#include <functional>
//#include <vector>
//
//namespace xx {
//
//    template<typename F = std::function<int()>>
//    struct TimeWheelManager : protected NodePool<std::pair<int, F>> {
//        using Base = NodePool<std::pair<int, F>>;
//        using Base::Base;
//        TimeWheelManager(TimeWheelManager const&) = delete;
//        TimeWheelManager& operator=(TimeWheelManager const&) = delete;
//        TimeWheelManager(TimeWheelManager&& o) noexcept {
//            operator=(std::move(o));
//        }
//        TimeWheelManager& operator=(TimeWheelManager&& o) noexcept {
//            this->Base::operator=(std::move(o));
//            std::swap(this->wheel, o.wheel);
//            std::swap(this->cursor, o.cursor);
//            return *this;
//        }
//
//        std::vector<int> wheel;
//        int cursor = 0;
//
//        // call once
//        void Init(int wheelSize, int nodeCapacity = 0) {
//            assert(wheel.empty());
//            assert(!this->Count());
//            wheel.resize(wheelSize, -1);
//            if (nodeCapacity) {
//                this->cap = nodeCapacity;
//                this->nodes = (typename Base::Node*)malloc(nodeCapacity * sizeof(typename Base::Node));
//            }
//        }
//
//        // call every frame
//        void Update() {
//            assert(!wheel.empty());
//            if ((++cursor) >= wheel.size()) cursor = 0;
//            int idx = wheel[cursor];
//            wheel[cursor] = -1;
//            while (idx >= 0) {
//                // locate & callback
//                auto& node = this->nodes[idx];
//                auto next = node.next;
//#ifndef NDEBUG
//                node.value.first = -1;
//#endif
//                auto timeoutSteps = node.value.second();
//                // kill?
//                if (timeoutSteps < 0) {
//                    this->Free(idx);
//                }
//                // set?
//                if (timeoutSteps > 0) {
//                    Set(idx, timeoutSteps);
//                }
//                // == 0: do nothing
//                idx = next;
//            }
//        }
//
//        void Kill(int idx) {
//            Remove(idx);
//            this->Free(idx);
//        }
//
//        template<typename FU>
//        int Create(int timeoutSteps, FU&& func) {
//            assert(!wheel.empty());
//            auto idx = this->Alloc(-1, std::forward<FU>(func));
//            Set(idx, timeoutSteps);
//            return idx;
//        }
//
//        void Move(int idx, int timeoutSteps) {
//            Remove(idx);
//            Set(idx, timeoutSteps);
//        }
//
//        using Base::Count;
//
//    protected:
//        void Remove(int idx) {
//            auto& node = (*this)[idx];
//            auto p = node.value.first;
//            assert(p >= 0);
//            if (wheel[p] == idx) {
//                wheel[p] = node.next;
//            }
//            if (node.prev >= 0) {
//                this->nodes[node.prev].next = node.next;
//            }
//            if (node.next >= 0) {
//                this->nodes[node.next].prev = node.prev;
//            }
//        }
//
//        void Set(int idx, int timeoutSteps) {
//            int ws = (int)wheel.size();
//            assert(timeoutSteps > 0 && timeoutSteps < ws);
//
//            // calc link's pos & locate
//            auto p = timeoutSteps + cursor;
//            if (p >= ws) p -= ws;
//            auto& node = this->nodes[idx];
//
//            // add to link header
//            node.prev = -1;
//            node.next = wheel[p];
//            node.value.first = p;
//            wheel[p] = idx;
//            if (node.next >= 0) {
//                this->nodes[node.next].prev = idx;
//            }
//        }
//
//    };
//}
//
//#include <iostream>
//int main() {
//    xx::TimeWheelManager<> m;
//    m.Init(100);
//    int t, c = 0;
//    t = m.Create(1, [&]()->int {
//        std::cout << "cb. c = " << c << std::endl;
//        switch (c) {
//            case 0:
//                ++c;
//                return 2;
//            case 1:
//                ++c;
//                return -1;
//            default:
//                return 0;
//        }
//    });
//    while (m.Count()) {
//        std::cout << "cursor = " << m.cursor << std::endl;
//        m.Update();
//    }
//	return 0;
//}


//#include <xx_bufholder.h>
//#include <xx_string.h>
//
//int main() {
//	struct XY { float x, y; };
//	struct XYZ : XY { float z; };
//	xx::BufHolder bh;
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//	bh.Get<XY>(2);
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//	bh.Get<XYZ>(2);
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//	bh.Get<XY>(3);
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//	bh.Get<char*>(4);
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//	bh.Shrink();
//	xx::CoutN("buf = ", (size_t)bh.buf, " cap = ", bh.cap);
//
//	return 0;
//}



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
