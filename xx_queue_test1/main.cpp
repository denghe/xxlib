#include <xx_podvector.h>
#include "pod_vector.h"
#include <xx_string.h>

int main() {
    for (size_t k = 0; k < 10; k++) {
    {
        auto secs = xx::NowEpochSeconds();
        uint64_t count = 0;
        for (size_t i = 0; i < 1000000; i++) {
            ax::pod_vector<int> pv;
            // pv.reserve(1000);
            for (int j = 0; j < 1000; j++) {
                pv.emplace_back(j);
            }
            auto buf = pv.release_pointer();
            auto bufKiller = xx::MakeSimpleScopeGuard([&] { delete[] buf; });
            for (int j = 0; j < 1000; j++) {
                count += buf[j];
            }
        }
        std::cout << count << " 0 " << xx::NowEpochSeconds(secs) << std::endl;
    }
    {
        auto secs = xx::NowEpochSeconds();
        uint64_t count = 0;
        for (size_t i = 0; i < 1000000; i++) {
            ax::pod_vector<int> pv;
            pv.reserve(1000);
            for (int j = 0; j < 1000; j++) {
                pv.emplace_back(j);
            }
            auto buf = pv.release_pointer();
            auto bufKiller = xx::MakeSimpleScopeGuard([&] { delete[] buf; });
            for (int j = 0; j < 1000; j++) {
                count += buf[j];
            }
        }
        std::cout << count << " 1 " << xx::NowEpochSeconds(secs) << std::endl;
    }
    {
        auto secs = xx::NowEpochSeconds();
        uint64_t count = 0;
        for (size_t i = 0; i < 1000000; i++) {
            xx::PodVector<int> pv;
            // pv.Reserve(1000);
            for (int j = 0; j < 1000; j++) {
                pv.Emplace(j);
            }
            auto buf = pv.TakeAwayBuf();
            auto bufKiller = xx::MakeSimpleScopeGuard([&] { delete[] buf; });
            for (int j = 0; j < 1000; j++) {
                count += buf[j];
            }
        }
        std::cout << count << " 2 " << xx::NowEpochSeconds(secs) << std::endl;
    }
    {
        auto secs = xx::NowEpochSeconds();
        uint64_t count = 0;
        for (size_t i = 0; i < 1000000; i++) {
            xx::PodVector<int> pv;
            pv.Reserve(1000);
            for (int j = 0; j < 1000; j++) {
                pv.Emplace(j);
            }
            auto buf = pv.TakeAwayBuf();
            auto bufKiller = xx::MakeSimpleScopeGuard([&] { delete[] buf; });
            for (int j = 0; j < 1000; j++) {
                count += buf[j];
            }
        }
        std::cout << count << " 3 " << xx::NowEpochSeconds(secs) << std::endl;
    }
    }
	std::cout << "end." << std::endl;
	return 0;
}




//#include "xx_string.h"
//#include "xx_ptr.h"
//#ifdef _WIN32MIMALLIC
//#include <mimalloc-new-delete.h>
//#endif
//
//#define NUM_ITEMS 100000
//#define UPDATE_TIMES 1000
//#define ENABLE_STRUCT_CONTEXT 0
//
//using TypeId = ptrdiff_t;
//using Counter = ptrdiff_t;
//
//struct Base {
//    static const TypeId __typeId__ = 0;
//    TypeId typeId_;
//    Counter counter = 0;
//#if ENABLE_STRUCT_CONTEXT
//    std::string name = "BASE";
//#endif
//    virtual void Update() = 0;
//    virtual ~Base() = default;
//};
//
//struct A : Base {
//    static const TypeId __typeId__ = 1;
//    A() {
//        this->typeId_ = __typeId__;
//    }
//#if ENABLE_STRUCT_CONTEXT
//    std::string name = "A";
//#endif
//    void Update() override { UpdateCore(); }
//    void UpdateCore() { counter++; }
//};
//struct B : Base {
//    static const TypeId __typeId__ = 2;
//    B() {
//        this->typeId_ = __typeId__;
//    }
//
//#if ENABLE_STRUCT_CONTEXT
//    std::string name = "B";
//    std::string desc = "BDESC";
//#endif
//    void Update() override { UpdateCore(); }
//    void UpdateCore() { counter++; }
//};
//struct C {
//    std::vector<std::shared_ptr<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                items[i]->Update();
//            }
//        }
//        else {
//            for (auto &o : items) o->Update();
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += o->counter;
//        return r;
//    }
//};
//
//struct C1 {
//    std::vector<xx::Shared<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                items[i]->Update();
//            }
//        }
//        else {
//            for (auto &o : items) o->Update();
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += o->counter;
//        return r;
//    }
//};
//
//struct C2 {
//    std::vector<xx::Shared<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                switch(items[i]->typeId_) {
//                    case A::__typeId__:
//                        ((A*)&*items[i])->UpdateCore();
//                        break;
//                    case B::__typeId__:
//                        ((B*)&*items[i])->UpdateCore();
//                        break;
//                }
//            }
//        }
//        else {
//            for (auto &o : items) {
//                switch(o->typeId_) {
//                    case 3:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 4:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 5:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 6:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 7:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 8:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 9:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 10:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 11:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 12:
//                        break;
//                        ((B*)&*o)->UpdateCore();
//                    case 13:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 14:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case A::__typeId__:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case B::__typeId__:
//                        ((B*)&*o)->UpdateCore();
//                        break;
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
//
//
//struct C3 {
//    std::vector<std::unique_ptr<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                items[i]->Update();
//            }
//        }
//        else {
//            for (auto &o : items) {
//                o->Update();
//            }
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += o->counter;
//        return r;
//    }
//};
//
//namespace xx {
//    template<typename T>
//    struct Unique {
//        using ElementType = T;
//        T *pointer = nullptr;
//
//        XX_INLINE operator T *const &() const noexcept {
//            return pointer;
//        }
//
//        XX_INLINE operator T *&() noexcept {
//            return pointer;
//        }
//
//        XX_INLINE T *const &operator->() const noexcept {
//            return pointer;
//        }
//
//        XX_INLINE T const &Value() const noexcept {
//            return *pointer;
//        }
//
//        XX_INLINE T &Value() noexcept {
//            return *pointer;
//        }
//
//        [[maybe_unused]] [[nodiscard]] XX_INLINE explicit operator bool() const noexcept {
//            return pointer != nullptr;
//        }
//
//        [[maybe_unused]] [[nodiscard]] XX_INLINE bool Empty() const noexcept {
//            return pointer == nullptr;
//        }
//
//        [[maybe_unused]] [[nodiscard]] XX_INLINE bool HasValue() const noexcept {
//            return pointer != nullptr;
//        }
//
//        void Reset() {
//            if (pointer) {
//                pointer->~T();
//                pointer = nullptr;
//            }
//        }
//
//        template<typename U>
//        void Reset(U *const &ptr) {
//            static_assert(std::is_same_v<T, U> || std::is_base_of_v<T, U>);
//            if (pointer == ptr) return;
//            Reset();
//            if (ptr) {
//                pointer = ptr;
//            }
//        }
//
//        XX_INLINE ~Unique() {
//            Reset();
//        }
//
//        Unique() = default;
//
//        template<typename U>
//        XX_INLINE Unique(U*const &ptr) {
//            static_assert(std::is_base_of_v<T, U>);
//            pointer = ptr;
//        }
//
//        XX_INLINE Unique(T*const &ptr) {
//            pointer = ptr;
//        }
//
//        XX_INLINE Unique(Unique const &o) = delete;
//
//        template<typename U>
//        XX_INLINE Unique(Unique<U> &&o) noexcept {
//            static_assert(std::is_base_of_v<T, U>);
//            pointer = o.pointer;
//            o.pointer = nullptr;
//        }
//
//        XX_INLINE Unique(Unique &&o) noexcept {
//            pointer = o.pointer;
//            o.pointer = nullptr;
//        }
//
//        template<typename U>
//        XX_INLINE Unique &operator=(U *const &ptr) {
//            static_assert(std::is_base_of_v<T, U>);
//            Reset(ptr);
//            return *this;
//        }
//
//        XX_INLINE Unique &operator=(T *const &ptr) {
//            Reset(ptr);
//            return *this;
//        }
//
//        XX_INLINE Unique &operator=(Unique const &o) = delete;
//
//        template<typename U>
//        XX_INLINE Unique &operator=(Unique<U> &&o) {
//            static_assert(std::is_base_of_v<T, U>);
//            Reset();
//            std::swap(pointer, (*(Unique * ) & o).pointer);
//            return *this;
//        }
//
//        XX_INLINE Unique &operator=(Unique &&o) {
//            std::swap(pointer, o.pointer);
//            return *this;
//        }
//
//        template<typename U>
//        XX_INLINE bool operator==(Unique<U> const &o) const noexcept {
//            return pointer == o.pointer;
//        }
//
//        template<typename U>
//        XX_INLINE bool operator!=(Unique<U> const &o) const noexcept {
//            return pointer != o.pointer;
//        }
//
//        // 有条件的话尽量使用 ObjManager 的 As, 避免发生 dynamic_cast
//        template<typename U>
//        XX_INLINE Unique<U> As() const noexcept {
//            if constexpr (std::is_same_v<U, T>) {
//                return *this;
//            } else if constexpr (std::is_base_of_v<U, T>) {
//                return pointer;
//            } else {
//                return dynamic_cast<U *>(pointer);
//            }
//        }
//
//        template<typename U>
//        XX_INLINE Unique<U> &ReinterpretCast() const noexcept {
//            return *(Unique<U> * )
//            this;
//        }
//
//        // 填充式 make
//        template<typename...Args>
//        Unique &Emplace(Args &&...args) {
//            Reset();
//            pointer = new T(std::forward<Args>(args)...);
//            return *this;
//        }
//    };
//
//    template<typename T, typename...Args>
//    [[maybe_unused]] [[nodiscard]] Unique<T> UMake(Args &&...args) {
//        Unique<T> rtv;
//        rtv.Emplace(std::forward<Args>(args)...);
//        return rtv;
//    }
//}
//
//
//struct C4 {
//    std::vector<xx::Unique<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                items[i]->Update();
//            }
//        }
//        else {
//            for (auto &o : items) {
//                o->Update();
//            }
//        }
//    }
//    Counter Sum() {
//        Counter r = 0;
//        for (auto& o : items) r += o->counter;
//        return r;
//    }
//};
//
//
//struct C5 {
//    std::vector<xx::Unique<Base>> items;
//    template<bool reverse = false>
//    void Update() {
//        if constexpr(reverse) {
//            for (int i = (int)items.size() - 1; i >= 0; --i) {
//                switch(items[i]->typeId_) {
//                    case A::__typeId__:
//                        ((A*)&*items[i])->UpdateCore();
//                        break;
//                    case B::__typeId__:
//                        ((B*)&*items[i])->UpdateCore();
//                        break;
//                }
//            }
//        }
//        else {
//            for (auto &o : items) {
//                switch(o->typeId_) {
//                    case 3:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 4:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 5:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 6:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 7:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 8:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 9:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 10:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case 11:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 12:
//                        break;
//                        ((B*)&*o)->UpdateCore();
//                    case 13:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case 14:
//                        ((B*)&*o)->UpdateCore();
//                        break;
//                    case A::__typeId__:
//                        ((A*)&*o)->UpdateCore();
//                        break;
//                    case B::__typeId__:
//                        ((B*)&*o)->UpdateCore();
//                        break;
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
//
//
////
////struct D {
////    Counter counter = 0;
////};
////struct E : D {
////    //std::string name = "E";
////    void Update() { counter++; }
////};
////struct F : D {
////    //std::string name = "F";
////    //std::string desc = "FDESC";
////    void Update() { counter--; }
////};
////struct G {
////    std::vector<std::shared_ptr<std::variant<E, F>>> items;
////    template<bool reverse = false>
////    void Update() {
////        struct MyVisitor {
////            void operator()(E& o) const { o.Update(); }
////            void operator()(F& o) const { o.Update(); }
////        };
////        if constexpr(reverse) {
////            for (int i = (int)items.size() - 1; i >= 0; --i) {
////                std::visit(MyVisitor(), *items[i]);
////            }
////        }
////        else {
////            for (auto &o : items)
////                std::visit(MyVisitor(), *o);
////        }
////    }
////    Counter Sum() {
////        struct MyVisitor {
////            Counter operator()(E& o) const { return o.counter; }
////            Counter operator()(F& o) const { return o.counter; }
////        };
////        Counter r = 0;
////        for (auto& o : items) r += std::visit(MyVisitor(), *o);
////        return r;
////    }
////};
////struct H {
////    std::vector<std::variant<E, F>> items;
////    template<bool reverse = false>
////    void Update() {
////        struct MyVisitor {
////            void operator()(E& o) const { o.Update(); }
////            void operator()(F& o) const { o.Update(); }
////        };
////        if constexpr(reverse) {
////            for (int i = (int)items.size() - 1; i >= 0; --i) {
////                std::visit(MyVisitor(), items[i]);
////            }
////        }
////        else {
////            for (auto &o : items)
////                std::visit(MyVisitor(), o);
////        }
////    }
////    Counter Sum() {
////        struct MyVisitor {
////            Counter operator()(E& o) const { return o.counter; }
////            Counter operator()(F& o) const { return o.counter; }
////        };
////        Counter r = 0;
////        for (auto& o : items) r += std::visit(MyVisitor(), o);
////        return r;
////    }
////};
////
////
////
////struct I {
////    TypeId typeId = 0; // J, K
////    Counter counter = 0;
////};
////struct J : I {
////    static const TypeId typeId = 1;
////    //std::string name = "J";
////    J() {
////        this->I::typeId = typeId;
////    }
////    void Update() { counter++; }
////};
////struct K : I {
////    static const TypeId typeId = 2;
////    //std::string name = "K";
////    //std::string desc = "KDESC";
////    K() {
////        this->I::typeId = typeId;
////    }
////    void Update() { counter--; }
////};
////struct L {
////    std::vector<std::shared_ptr<I>> items;
////    template<bool reverse = false>
////    void Update() {
////        if constexpr(reverse) {
////            for (int i = (int)items.size() - 1; i >= 0; --i) {
////                auto& o = items[i];
////                switch (o->typeId) {
////                    case J::typeId: ((J*)&*o)->Update(); break;
////                    case K::typeId: ((K*)&*o)->Update(); break;
////                }
////            }
////        }
////        else {
////            for (auto& o : items) {
////                switch (o->typeId) {
////                    case J::typeId: ((J*)&*o)->Update(); break;
////                    case K::typeId: ((K*)&*o)->Update(); break;
////                }
////            }
////        }
////    }
////    Counter Sum() {
////        Counter r = 0;
////        for (auto& o : items) r += o->counter;
////        return r;
////    }
////};
////union M {
////    I i;
////    J j;
////    K k;
////    void Update() {
////        switch (i.typeId) {
////            case J::typeId: j.Update(); break;
////            case K::typeId: k.Update(); break;
////        }
////    }
////    void Dispose() {
////        switch (i.typeId) {
////            case 0: break;
////            case J::typeId: j.~J(); break;
////            case K::typeId: k.~K(); break;
////        }
////    }
////};
////struct MS {
////    std::aligned_storage<sizeof(M), 8>::type store;
////    MS() {
////        memset(&store, 0, sizeof(I::typeId));
////    }
////    M* operator->() const noexcept {
////        return (M*)&store;
////    }
////    ~MS() {
////        (*this)->Dispose();
////        memset(&store, 0, sizeof(I::typeId));
////    }
////};
////
////struct N {
////    std::vector<MS> items;
////    template<bool reverse = false>
////    void Update() {
////        if constexpr(reverse) {
////            for (int i = (int)items.size() - 1; i >= 0; --i) {
////                items[i]->Update();
////            }
////        }
////        else {
////            for (auto& o : items) {
////                o->Update();
////            }
////        }
////    }
////    Counter Sum() {
////        Counter r = 0;
////        for (auto& o : items) r += (*(M*)&o).i.counter;
////        return r;
////    }
////};
////
////
////
////typedef void(*Func)(void*);
////struct O {
////    ptrdiff_t counter = 0;
////    Func update;
////};
////struct P : O {
////    P() {
////        update = [](void* o){ ((P*)o)->Update(); };
////    }
////    void Update() {
////        counter++;
////    }
////};
////struct Q : O {
////    Q() {
////        update = [](void* o){ ((Q*)o)->Update(); };
////    }
////    void Update() {
////        counter--;
////    }
////};
////union R {
////    O o;
////    P p;
////    Q q;
////};
////using RS = typename std::aligned_storage<sizeof(R), 8>::type;
////struct S {
////    std::vector<RS> items;
////    template<bool reverse = false>
////    void Update() {
////        if constexpr (reverse) {
////            for (int i = (int)items.size() - 1; i >= 0; --i) {
////                auto& o = items[i];
////                (*(*(R*)&o).o.update)(&o);
////            }
////        }
////        else {
////            for (auto& o : items) {
////                (*(*(R*)&o).o.update)(&o);
////            }
////        }
////    }
////    ptrdiff_t Sum() {
////        ptrdiff_t r = 0;
////        for (auto& o : items) r += (*(R*)&o).o.counter;
////        return r;
////    }
////};
//
//
//
//
//template<bool reverse = false, typename T>
//void test(T& c, std::string_view prefix) {
//    auto secs = xx::NowEpochSeconds();
//    for (int i = 0; i < UPDATE_TIMES; ++i) {
//        c.template Update<reverse>();
//    }
//    xx::CoutN(prefix, " update ", UPDATE_TIMES, " times, secs = ", xx::NowEpochSeconds() - secs);
//
//    secs = xx::NowEpochSeconds();
//    auto counter = c.Sum();
//    xx::CoutN(prefix, " counter = ", counter, ", secs = ", xx::NowEpochSeconds() - secs);
//}
//
//void test1() {
//    C c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(std::make_shared<A>());
//        c.items.emplace_back(std::make_shared<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1 normal ");
//    }
//}
//void test1a() {
//    C1 c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(xx::Make<A>());
//        c.items.emplace_back(xx::Make<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1a std::shared_ptr -> xx::Shared ");
//    }
//}
//void test1b() {
//    C2 c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(xx::Make<A>());
//        c.items.emplace_back(xx::Make<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1b std::shared_ptr -> xx::Shared & switch case");
//    }
//}
//void test1c() {
//    C3 c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(std::make_unique<A>());
//        c.items.emplace_back(std::make_unique<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1c std::shared_ptr -> std::unique_ptr");
//    }
//}
//void test1d() {
//    C4 c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(xx::UMake<A>());
//        c.items.emplace_back(xx::UMake<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1d std::shared_ptr -> xx::Unique");
//    }
//}
//void test1e() {
//    C5 c;
//    for (int i = 0; i < NUM_ITEMS; ++i) {
//        c.items.emplace_back(xx::UMake<A>());
//        c.items.emplace_back(xx::UMake<B>());
//    }
//    for (int i = 0; i <10; ++i) {
//        test(c, "test1d std::shared_ptr -> xx::Unique & switch case");
//    }
//}
//
////void test2() {
////    G c;
////    for (int i = 0; i < NUM_ITEMS; ++i) {
////        c.items.emplace_back(std::make_shared<std::variant<E, F>>(E{}));
////        c.items.emplace_back(std::make_shared<std::variant<E, F>>(F{}));
////    }
////    test(c, "test2");
////    test<true>(c, "test2 reverse");
////}
////void test3() {
////    H c;
////    for (int i = 0; i < NUM_ITEMS; ++i) {
////        c.items.emplace_back(E{});
////        c.items.emplace_back(F{});
////    }
////    test(c, "test3");
////    test<true>(c, "test3 reverse");
////}
////void test4() {
////    L c;
////    for (int i = 0; i < NUM_ITEMS; ++i) {
////        c.items.emplace_back(std::make_shared<J>());
////        c.items.emplace_back(std::make_shared<K>());
////    }
////    test(c, "test4");
////    test<true>(c, "test4 reverse");
////}
////void test5() {
////    N c;
////    c.items.reserve(NUM_ITEMS);
////    for (int i = 0; i < NUM_ITEMS; ++i) {
////        new (&c.items.emplace_back()) J();
////        new (&c.items.emplace_back()) K();
////    }
////    test(c, "test5");
////    test<true>(c, "test5 reverse");
////}
////void test6() {
////    S c;
////    c.items.reserve(NUM_ITEMS);
////    for (int i = 0; i < NUM_ITEMS; ++i) {
////        new (&c.items.emplace_back()) P();
////        new (&c.items.emplace_back()) Q();
////    }
////    test(c, "test6");
////    test<true>(c, "test6 reverse");
////}
//
//int main() {
//    //for (int i = 0; i <10; ++i) {
//        test1();
//        test1a();
//        test1b();
//        test1c();
//        test1d();
//        test1e();
////        test2();
////        test3();
////        test4();
////        test5();
////        test6();
//    //}
//	return 0;
//}
//














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
