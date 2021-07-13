#include "xx_string.h"

#define REVERSE_FOR

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
    template<bool reverse = false>
    void Update() {
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                items[i]->Update();
            }
        }
        else {
            for (auto &o : items) o->Update();
        }
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
    template<bool reverse = false>
    void Update() {
        struct MyVisitor {
            void operator()(E& o) const { o.Update(); }
            void operator()(F& o) const { o.Update(); }
        };
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                std::visit(MyVisitor(), *items[i]);
            }
        }
        else {
            for (auto &o : items)
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
    template<bool reverse = false>
    void Update() {
        struct MyVisitor {
            void operator()(E& o) const { o.Update(); }
            void operator()(F& o) const { o.Update(); }
        };
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                std::visit(MyVisitor(), items[i]);
            }
        }
        else {
            for (auto &o : items)
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
    J() {
        this->I::typeId = typeId;
    }
    void Update() { counter++; }
};
struct K : I {
    static const ptrdiff_t typeId = 2;
    K() {
        this->I::typeId = typeId;
    }
    void Update() { counter--; }
};
struct L {
    std::vector<std::shared_ptr<I>> items;
    template<bool reverse = false>
    void Update() {
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                auto& o = items[i];
                switch (o->typeId) {
                    case J::typeId: ((J*)&*o)->Update(); break;
                    case K::typeId: ((K*)&*o)->Update(); break;
                }
            }
        }
        else {
            for (auto& o : items) {
                switch (o->typeId) {
                    case J::typeId: ((J*)&*o)->Update(); break;
                    case K::typeId: ((K*)&*o)->Update(); break;
                }
            }
        }
    }
    ptrdiff_t Sum() {
        ptrdiff_t r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};
union M {
    I i;
    J j;
    K k;
};
using MS = typename std::aligned_storage<sizeof(M), 8>::type;

struct N {
    std::vector<MS> items;
    template<bool reverse = false>
    void Update() {
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                auto& o = items[i];
                switch ((*(M*)&o).i.typeId) {
                    case J::typeId: (*(M*)&o).j.Update(); break;
                    case K::typeId: (*(M*)&o).k.Update(); break;
                }
            }
        }
        else {
            for (auto& o : items) {
                switch ((*(M*)&o).i.typeId) {
                    case J::typeId: (*(M*)&o).j.Update(); break;
                    case K::typeId: (*(M*)&o).k.Update(); break;
                }
            }
        }
    }
    ptrdiff_t Sum() {
        ptrdiff_t r = 0;
        for (auto& o : items) r += (*(M*)&o).i.counter;
        return r;
    }
};






template<bool reverse = false, typename T>
void test(T& c, std::string_view prefix) {
    auto secs = xx::NowEpochSeconds();
    for (int i = 0; i < 10000; ++i) {
        c.template Update<reverse>();
    }
    xx::CoutN(prefix, xx::NowEpochSeconds() - secs);

    secs = xx::NowEpochSeconds();
    auto counter = c.Sum();
    xx::CoutN(prefix, " ", counter, " ", xx::NowEpochSeconds() - secs);
}

void test1() {
    C c;
    for (int i = 0; i < 100000; ++i) {
        c.items.emplace_back(std::make_shared<A>());
        c.items.emplace_back(std::make_shared<B>());
    }
    test(c, "test1");
    test<true>(c, "test1 reverse");
}

void test2() {
    G c;
    for (int i = 0; i < 100000; ++i) {
        c.items.emplace_back(std::make_shared<std::variant<E, F>>(E{}));
        c.items.emplace_back(std::make_shared<std::variant<E, F>>(F{}));
    }
    test(c, "test2");
    test<true>(c, "test2 reverse");
}

void test3() {
    H c;
    for (int i = 0; i < 100000; ++i) {
        c.items.emplace_back(E{});
        c.items.emplace_back(F{});
    }
    test(c, "test3");
    test<true>(c, "test3 reverse");
}
void test4() {
    L c;
    for (int i = 0; i < 100000; ++i) {
        c.items.emplace_back(std::make_shared<J>());
        c.items.emplace_back(std::make_shared<K>());
    }
    test(c, "test4");
    test<true>(c, "test4 reverse");
}
void test5() {
    N c;
    for (int i = 0; i < 100000; ++i) {
        new (&c.items.emplace_back()) J();
        new (&c.items.emplace_back()) K();
    }
    test(c, "test5");
    test<true>(c, "test5 reverse");
}

int main() {
    for (int i = 0; i <10; ++i) {
        test1();
        test2();
        test3();
        test4();
        test5();
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
