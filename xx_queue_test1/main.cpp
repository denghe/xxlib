#include "xx_string.h"

struct Base {
    ptrdiff_t counter = 0;
    virtual void Update() = 0;
    virtual ~Base() = default;
};
struct A : Base {
    void Update() override { counter++; }
};
struct B : Base {
    void Update() override { counter--; }
};
struct C {
    std::vector<std::shared_ptr<Base>> items;
    void Update() {
        for(auto& o : items) o->Update();
    }
    ptrdiff_t Sum() {
        ptrdiff_t r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};


struct D {
    ptrdiff_t counter = 0;
};
struct E : D {
    void Update() { counter++; }
};
struct F : D {
    void Update() { counter--; }
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
    ptrdiff_t Sum() {
        struct MyVisitor {
            ptrdiff_t operator()(E& o) const { return o.counter; }
            ptrdiff_t operator()(F& o) const { return o.counter; }
        };
        ptrdiff_t r = 0;
        for (auto& o : items) r += std::visit(MyVisitor(), *o);
        return r;
    }
};
struct H {
    std::vector<std::variant<E, F>> items;
    void Update() {
        struct MyVisitor {
            void operator()(E& o) const { o.Update(); }
            void operator()(F& o) const { o.Update(); }
        };
        for (auto& o : items) {
            std::visit(MyVisitor(), o);
        }
    }
    ptrdiff_t Sum() {
        struct MyVisitor {
            ptrdiff_t operator()(E& o) const { return o.counter; }
            ptrdiff_t operator()(F& o) const { return o.counter; }
        };
        ptrdiff_t r = 0;
        for (auto& o : items) r += std::visit(MyVisitor(), o);
        return r;
    }
};



struct I {
    ptrdiff_t typeId = 0; // J, K
    ptrdiff_t counter = 0;
};
struct J : I {
    static const ptrdiff_t typeId = 1;
    void Update() { counter++; }
};
struct K : I {
    static const ptrdiff_t typeId = 2;
    void Update() { counter--; }
};
struct L {
    std::vector<std::shared_ptr<I>> items;
    void Update() {
        for (auto& o : items) {
            switch (o->typeId) {
            case J::typeId: ((J*)&*o)->Update(); break;
            case K::typeId: ((K*)&*o)->Update(); break;
            }
        }
    }
    ptrdiff_t Sum() {
        ptrdiff_t r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};






template<typename T>
void test(T& c, std::string_view prefix) {
    auto secs = xx::NowEpochSeconds();
    for (int i = 0; i < 1000000; ++i) {
        c.Update();
    }
    xx::CoutN(prefix, xx::NowEpochSeconds() - secs);

    secs = xx::NowEpochSeconds();
    auto counter = c.Sum();
    xx::CoutN(prefix, " ", counter, " ", xx::NowEpochSeconds() - secs);
}

void test1() {
    C c;
    for (int i = 0; i < 1000; ++i) {
        c.items.emplace_back(std::make_shared<A>());
        c.items.emplace_back(std::make_shared<B>());
    }
    test(c, "test1");
}

void test2() {
    G c;
    for (int i = 0; i < 1000; ++i) {
        c.items.emplace_back(std::make_shared<std::variant<E, F>>(E{}));
        c.items.emplace_back(std::make_shared<std::variant<E, F>>(F{}));
    }
    test(c, "test2");
}

void test3() {
    H c;
    for (int i = 0; i < 1000; ++i) {
        c.items.emplace_back(E{});
        c.items.emplace_back(F{});
    }
    test(c, "test3");
}
void test4() {
    L c;
    for (int i = 0; i < 1000; ++i) {
        c.items.emplace_back(std::make_shared<J>())->typeId = J::typeId;
        c.items.emplace_back(std::make_shared<K>())->typeId = K::typeId;
    }
    test(c, "test4");
}

int main() {
    for (int i = 0; i <10; ++i) {
        test1();
        test2();
        test3();
        test4();
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
