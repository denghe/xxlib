#include "xx_string.h"

struct Base {
    ptrdiff_t* counter = nullptr;
    virtual void Update() = 0;
    virtual ~Base() = default;
};
struct A : Base {
    void Update() override { (*counter)++; }
};
struct B : Base {
    void Update() override { (*counter)--; }
};
struct C {
    std::vector<std::shared_ptr<Base>> items;
    void Update() {
        for(auto& o : items) o->Update();
    }
};

void test1() {
    ptrdiff_t counter = 0;
    C c;
    for (int i = 0; i < 1000; ++i) {
        c.items.emplace_back(std::make_shared<A>())->counter = &counter;
        c.items.emplace_back(std::make_shared<B>())->counter = &counter;
    }

    auto secs = xx::NowEpochSeconds();
    for (int i = 0; i < 1000000; ++i) {
        c.Update();
    }
    xx::CoutN("sharedptr ", counter, " ", xx::NowEpochSeconds() - secs);
}


struct D {
    ptrdiff_t* counter = nullptr;
};
struct E : D {
    void Update() { (*counter)++; }
};
struct F : D {
    void Update() { (*counter)--; }
};
struct G {
    std::vector<std::shared_ptr<std::variant<E, F>>> items;
    void Update() {
        struct MyVisitor {
            void operator()(E& o) const { o.Update(); }
            void operator()(F& o) const { o.Update(); }
        };
        for (auto& o : items) {
            std::visit(MyVisitor(), *o);
        }
    }
};

void test2() {
    ptrdiff_t counter = 0;
    G c;
    for (int i = 0; i < 1000; ++i) {
        std::get<E>(*c.items.emplace_back(std::make_shared<std::variant<E, F>>(E{}))).counter = &counter;
        std::get<F>(*c.items.emplace_back(std::make_shared<std::variant<E, F>>(F{}))).counter = &counter;
    }

    auto secs = xx::NowEpochSeconds();
    for (int i = 0; i < 1000000; ++i) {
        c.Update();
    }
    xx::CoutN("variant ", counter, " ", xx::NowEpochSeconds() - secs);
}

int main() {
    for (int i = 0; i <10; ++i) {
        test1();
        test2();
    }
	return 0;
}















//#include "xx_queue.h"
//#include <iostream>
//
//int main() {
//	xx::Queue<int> q;
//	q.Push(1, 2, 3);
//	std::cout << q.Count() << std::endl;
//	std::cout << q[0] << std::endl;
//	std::cout << q[1] << std::endl;
//	std::cout << q[2] << std::endl;
//	q.PopMulti(2);
//	std::cout << q.Count() << std::endl;
//
//	std::cout << "end." << std::endl;
//	return 0;
//}
