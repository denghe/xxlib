#include "xx_dict_mk.h"
#include <iostream>
#include "xx_ptr.h"

int main() {
    using T = std::tuple<int, std::string>;
    using U = xx::Shared<T>;
	auto t1 = xx::Make<T>(1, "a");
	auto t2 = xx::Make<T>(2, "b");
	auto t3 = xx::Make<T>(3, "c");
	auto t4 = xx::Make<T>(2, "d");
	auto t5 = xx::Make<T>(4, "a");

    xx::DictMK<U, int, std::string> d;
	auto r = d.Add(t1, std::get<0>(*t1), std::get<1>(*t1));
	assert(r.success);
	assert(d.KeyAt<0>(r.index) == std::get<0>(*t1));
	assert(d.KeyAt<1>(r.index) == std::get<1>(*t1));
	assert(d.ValueAt(r.index) == t1);
	r = d.Add(t2, std::get<0>(*t2), std::get<1>(*t2));
    assert(r.success);
    assert(d.KeyAt<0>(r.index) == std::get<0>(*t2));
    assert(d.KeyAt<1>(r.index) == std::get<1>(*t2));
    assert(d.ValueAt(r.index) == t2);
	r = d.Add(t3, std::get<0>(*t3), std::get<1>(*t3));
    assert(r.success);
    assert(d.KeyAt<0>(r.index) == std::get<0>(*t3));
    assert(d.KeyAt<1>(r.index) == std::get<1>(*t3));
    assert(d.ValueAt(r.index) == t3);
    assert(d.Count() == 3);
    assert(std::get<1>(d.dicts).Count() == d.dict.Count());

    U t;
    auto b = d.TryGetValue<1>("c", t);
    assert(b);
    assert(t == t3);

    r = d.Add(t4, std::get<0>(*t4), std::get<1>(*t4));
    assert(!r.success);
    assert(d.Count() == 3);
    assert(std::get<1>(d.dicts).Count() == d.dict.Count());
    r = d.Add(t5, std::get<0>(*t4), std::get<1>(*t5));
    assert(!r.success);
    assert(d.Count() == 3);
    assert(std::get<1>(d.dicts).Count() == d.dict.Count());

    auto idx = d.Find<1>(std::get<1>(*t1));
    assert(idx != -1);
    assert(idx == d.Find<0>(std::get<0>(*t1)));

    idx = d.Find<0>(std::get<0>(*t2));
    assert(idx != -1);
    assert(idx == d.Find<1>(std::get<1>(*t2)));

    idx = d.Find<1>(std::get<1>(*t3));
    assert(idx != -1);
    assert(idx == d.Find<0>(std::get<0>(*t3)));

    for(auto& node : d.dict) {
        std::cout << std::get<0>(*node.value) << " " << std::get<1>(*node.value) << std::endl;
    }
    std::cout << "---------------------------- " << d.Count() << std::endl;

    auto ok = d.Remove<0>(std::get<0>(*t1));
    assert(ok);
    ok = d.Remove<1>(std::get<1>(*t1));
    assert(!ok);

	for(auto& node : d.dict) {
	    std::cout << std::get<0>(*node.value) << " " << std::get<1>(*node.value) << std::endl;
	}
    std::cout << "---------------------------- " << d.Count() << std::endl;

    ok = d.Remove<1>(std::get<1>(*t2));
    assert(ok);

	for(auto& node : d.dict) {
	    std::cout << std::get<0>(*node.value) << " " << std::get<1>(*node.value) << std::endl;
	}
    std::cout << "---------------------------- " << d.Count() << std::endl;

	d.Clear();
    std::cout << "---------------------------- " << d.Count() << std::endl;

	return 0;
}
