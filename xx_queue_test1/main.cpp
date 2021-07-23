#include "xx_string.h"

#define NUM_ITEMS 100000
#define UPDATE_TIMES 1000

using TypeId = ptrdiff_t;
using Counter = ptrdiff_t;


struct Base {
    static const TypeId __typeId__ = 0;
    TypeId typeId_;
    Counter counter = 0;
    virtual void Update() = 0;
    virtual ~Base() = default;
};

struct A : Base {
    static const TypeId __typeId__ = 1;
    A() {
        this->typeId_ = __typeId__;
    }
    //std::string name = "A";
    void Update() override { UpdateCore(); }
    void UpdateCore() { counter++; }
};
struct B : Base {
    static const TypeId __typeId__ = 2;
    B() {
        this->typeId_ = __typeId__;
    }
    //std::string name = "B";
    //std::string desc = "BDESC";
    void Update() override { UpdateCore(); }
    void UpdateCore() { counter++; }
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
    Counter Sum() {
        Counter r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};

#include "xx_ptr.h"


struct C1 {
    std::vector<xx::Shared<Base>> items;
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
    Counter Sum() {
        Counter r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};


struct C2 {
    std::vector<xx::Shared<Base>> items;
    template<bool reverse = false>
    void Update() {
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                switch(items[i]->typeId_) {
                    case A::__typeId__:
                        ((A*)&*items[i])->UpdateCore();
                        break;
                    case B::__typeId__:
                        ((B*)&*items[i])->UpdateCore();
                        break;
                }
            }
        }
        else {
            for (auto &o : items) {
                switch(o->typeId_) {
                    case A::__typeId__:
                        ((A*)&*o)->UpdateCore();
                        break;
                    case B::__typeId__:
                        ((B*)&*o)->UpdateCore();
                        break;
                }
            }
        }
    }
    Counter Sum() {
        Counter r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};


struct C3 {
    std::vector<std::unique_ptr<Base>> items;
    template<bool reverse = false>
    void Update() {
        if constexpr(reverse) {
            for (int i = (int)items.size() - 1; i >= 0; --i) {
                items[i]->Update();
            }
        }
        else {
            for (auto &o : items) {
                o->Update();
            }
        }
    }
    Counter Sum() {
        Counter r = 0;
        for (auto& o : items) r += o->counter;
        return r;
    }
};


//
//struct D {
//    Counter counter = 0;
//};
//struct E : D {
//    //std::string name = "E";
//    void Update() { counter++; }
//};
//struct F : D {
//    //std::string name = "F";
//    //std::string desc = "FDESC";
//    void Update() { counter--; }
//};
//struct G {
//    std::vector<std::shared_ptr<std::variant<E, F>>> items;
//    template<bool reverse = false>
//    void Update() {
//        struct MyVisitor {
//            void operator()(E& o) const { o.Update(); }
//            void operator()(F& o) const { o.Update(); }
//        };
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                std::visit(MyVisitor(), *items[i]);
//            }
//        }
//        else {
//            for (auto &o : items)
//                std::visit(MyVisitor(), *o);
//        }
//    }
//    Counter Sum() {
//        struct MyVisitor {
//            Counter operator()(E& o) const { return o.counter; }
//            Counter operator()(F& o) const { return o.counter; }
//        };
//        Counter r = 0;
//        for (auto& o : items) r += std::visit(MyVisitor(), *o);
//        return r;
//    }
//};
//struct H {
//    std::vector<std::variant<E, F>> items;
//    template<bool reverse = false>
//    void Update() {
//        struct MyVisitor {
//            void operator()(E& o) const { o.Update(); }
//            void operator()(F& o) const { o.Update(); }
//        };
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                std::visit(MyVisitor(), items[i]);
//            }
//        }
//        else {
//            for (auto &o : items)
//                std::visit(MyVisitor(), o);
//        }
//    }
//    Counter Sum() {
//        struct MyVisitor {
//            Counter operator()(E& o) const { return o.counter; }
//            Counter operator()(F& o) const { return o.counter; }
//        };
//        Counter r = 0;
//        for (auto& o : items) r += std::visit(MyVisitor(), o);
//        return r;
//    }
//};
//
//
//
//struct I {
//    TypeId typeId = 0; // J, K
//    Counter counter = 0;
//};
//struct J : I {
//    static const TypeId typeId = 1;
//    //std::string name = "J";
//    J() {
//        this->I::typeId = typeId;
//    }
//    void Update() { counter++; }
//};
//struct K : I {
//    static const TypeId typeId = 2;
//    //std::string name = "K";
//    //std::string desc = "KDESC";
//    K() {
//        this->I::typeId = typeId;
//    }
//    void Update() { counter--; }
//};
//struct L {
//    std::vector<std::shared_ptr<I>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                auto& o = items[i];
//                switch (o->typeId) {
//                    case J::typeId: ((J*)&*o)->Update(); break;
//                    case K::typeId: ((K*)&*o)->Update(); break;
//                }
//            }
//        }
//        else {
//            for (auto& o : items) {
//                switch (o->typeId) {
//                    case J::typeId: ((J*)&*o)->Update(); break;
//                    case K::typeId: ((K*)&*o)->Update(); break;
//                }
//            }
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += o->counter;
//        return r;
//    }
//};
//union M {
//    I i;
//    J j;
//    K k;
//    void Update() {
//        switch (i.typeId) {
//            case J::typeId: j.Update(); break;
//            case K::typeId: k.Update(); break;
//        }
//    }
//    void Dispose() {
//        switch (i.typeId) {
//            case 0: break;
//            case J::typeId: j.~J(); break;
//            case K::typeId: k.~K(); break;
//        }
//    }
//};
//struct MS {
//    std::aligned_storage<sizeof(M), 8>::type store;
//    MS() {
//        memset(&store, 0, sizeof(I::typeId));
//    }
//    M* operator->() const noexcept {
//        return (M*)&store;
//    }
//    ~MS() {
//        (*this)->Dispose();
//        memset(&store, 0, sizeof(I::typeId));
//    }
//};
//
//struct N {
//    std::vector<MS> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                items[i]->Update();
//            }
//        }
//        else {
//            for (auto& o : items) {
//                o->Update();
//            }
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += (*(M*)&o).i.counter;
//        return r;
//    }
//};
//
//
//
//typedef void(*Func)(void*);
//struct O {
//    ptrdiff_t counter = 0;
//    Func update;
//};
//struct P : O {
//    P() {
//        update = [](void* o){ ((P*)o)->Update(); };
//    }
//    void Update() {
//        counter++;
//    }
//};
//struct Q : O {
//    Q() {
//        update = [](void* o){ ((Q*)o)->Update(); };
//    }
//    void Update() {
//        counter--;
//    }
//};
//union R {
//    O o;
//    P p;
//    Q q;
//};
//using RS = typename std::aligned_storage<sizeof(R), 8>::type;
//struct S {
//    std::vector<RS> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr (reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                auto& o = items[i];
//                (*(*(R*)&o).o.update)(&o);
//            }
//        }
//        else {
//            for (auto& o : items) {
//                (*(*(R*)&o).o.update)(&o);
//            }
//        }
//    }
//    ptrdiff_t Sum() {
//        ptrdiff_t r = 0;
//        for (auto& o : items) r += (*(R*)&o).o.counter;
//        return r;
//    }
//};




template<bool reverse = false, typename T>
void test(T& c, std::string_view prefix) {
    auto secs = xx::NowEpochSeconds();
    for (int i = 0; i < UPDATE_TIMES; ++i) {
        c.template Update<reverse>();
    }
    xx::CoutN(prefix, " update ", UPDATE_TIMES, " times, secs = ", xx::NowEpochSeconds() - secs);

    secs = xx::NowEpochSeconds();
    auto counter = c.Sum();
    xx::CoutN(prefix, " counter = ", counter, ", secs = ", xx::NowEpochSeconds() - secs);
}

void test1() {
    C c;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        c.items.emplace_back(std::make_shared<A>());
        c.items.emplace_back(std::make_shared<B>());
    }
    for (int i = 0; i <10; ++i) {
        test(c, "test1 normal ");
    }
}
void test1a() {
    C2 c;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        c.items.emplace_back(xx::Make<A>());
        c.items.emplace_back(xx::Make<B>());
    }
    for (int i = 0; i <10; ++i) {
        test(c, "test1a std::shared_ptr -> xx::Shared ");
    }
}
void test1b() {
    C2 c;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        c.items.emplace_back(xx::Make<A>());
        c.items.emplace_back(xx::Make<B>());
    }
    for (int i = 0; i <10; ++i) {
        test(c, "test1b std::shared_ptr -> xx::Shared & switch case");
    }
}
void test1c() {
    C3 c;
    for (int i = 0; i < NUM_ITEMS; ++i) {
        c.items.emplace_back(std::make_unique<A>());
        c.items.emplace_back(std::make_unique<B>());
    }
    for (int i = 0; i <10; ++i) {
        test(c, "test1c std::shared_ptr -> std::unique_ptr");
    }
}

//void test2() {
//    G c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(std::make_shared<std::variant<E, F>>(E{}));
//        c.items.emplace_back(std::make_shared<std::variant<E, F>>(F{}));
//    }
//    test(c, "test2");
//    test<true>(c, "test2 reverse");
//}
//void test3() {
//    H c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(E{});
//        c.items.emplace_back(F{});
//    }
//    test(c, "test3");
//    test<true>(c, "test3 reverse");
//}
//void test4() {
//    L c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(std::make_shared<J>());
//        c.items.emplace_back(std::make_shared<K>());
//    }
//    test(c, "test4");
//    test<true>(c, "test4 reverse");
//}
//void test5() {
//    N c;
//    c.items.reserve(NUM_ITEMS);
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        new (&c.items.emplace_back()) J();
//        new (&c.items.emplace_back()) K();
//    }
//    test(c, "test5");
//    test<true>(c, "test5 reverse");
//}
//void test6() {
//    S c;
//    c.items.reserve(NUM_ITEMS);
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        new (&c.items.emplace_back()) P();
//        new (&c.items.emplace_back()) Q();
//    }
//    test(c, "test6");
//    test<true>(c, "test6 reverse");
//}

int main() {
    //for (int i = 0; i <10; ++i) {
        test1();
        test1a();
        test1b();
        test1c();
//        test2();
//        test3();
//        test4();
//        test5();
//        test6();
    //}
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
