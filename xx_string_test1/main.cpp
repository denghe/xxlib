#include <xx_nodepool.h>
#include <functional>
#include <vector>

namespace xx {

    template<typename F = std::function<int()>>
    struct TimeWheelManager : protected NodePool<std::pair<int, F>> {
        using Base = NodePool<std::pair<int, F>>;
        using Base::Base;
        TimeWheelManager(TimeWheelManager const&) = delete;
        TimeWheelManager& operator=(TimeWheelManager const&) = delete;
        TimeWheelManager(TimeWheelManager&& o) noexcept {
            operator=(std::move(o));
        }
        TimeWheelManager& operator=(TimeWheelManager&& o) noexcept {
            this->Base::operator=(std::move(o));
            std::swap(this->wheel, o.wheel);
            std::swap(this->cursor, o.cursor);
            return *this;
        }

        std::vector<int> wheel;
        int cursor = 0;

        // call once
        void Init(int wheelSize, int nodeCapacity = 0) {
            assert(wheel.empty());
            assert(!this->Count());
            wheel.resize(wheelSize, -1);
            if (nodeCapacity) {
                this->cap = nodeCapacity;
                this->nodes = (typename Base::Node*)malloc(nodeCapacity * sizeof(typename Base::Node));
            }
        }

        // call every frame
        void Update() {
            assert(!wheel.empty());
            if ((++cursor) >= wheel.size()) cursor = 0;
            int idx = wheel[cursor];
            wheel[cursor] = -1;
            while (idx >= 0) {
                // locate & callback
                auto& node = this->nodes[idx];
                auto next = node.next;
#ifndef NDEBUG
                node.value.first = -1;
#endif
                auto timeoutSteps = node.value.second();
                // kill?
                if (timeoutSteps < 0) {
                    this->Free(idx);
                }
                // set?
                if (timeoutSteps > 0) {
                    Set(idx, timeoutSteps);
                }
                // == 0: do nothing
                idx = next;
            }
        }

        void Kill(int idx) {
            Remove(idx);
            this->Free(idx);
        }

        template<typename FU>
        int Create(int timeoutSteps, FU&& func) {
            assert(!wheel.empty());
            auto idx = this->Alloc(-1, std::forward<FU>(func));
            Set(idx, timeoutSteps);
            return idx;
        }

        void Move(int idx, int timeoutSteps) {
            Remove(idx);
            Set(idx, timeoutSteps);
        }

        using Base::Count;

    protected:
        void Remove(int idx) {
            auto& node = (*this)[idx];
            auto p = node.value.first;
            assert(p >= 0);
            if (wheel[p] == idx) {
                wheel[p] = node.next;
            }
            if (node.prev >= 0) {
                this->nodes[node.prev].next = node.next;
            }
            if (node.next >= 0) {
                this->nodes[node.next].prev = node.prev;
            }
        }

        void Set(int idx, int timeoutSteps) {
            int ws = (int)wheel.size();
            assert(timeoutSteps > 0 && timeoutSteps < ws);

            // calc link's pos & locate
            auto p = timeoutSteps + cursor;
            if (p >= ws) p -= ws;
            auto& node = this->nodes[idx];

            // add to link header
            node.prev = -1;
            node.next = wheel[p];
            node.value.first = p;
            wheel[p] = idx;
            if (node.next >= 0) {
                this->nodes[node.next].prev = idx;
            }
        }

    };
}

#include <iostream>
int main() {
    xx::TimeWheelManager<> m;
    m.Init(100);
    int t, c = 0;
    t = m.Create(1, [&]()->int {
        std::cout << "cb. c = " << c << std::endl;
        switch (c) {
            case 0:
                ++c;
                return 2;
            case 1:
                ++c;
                return -1;
            default:
                return 0;
        }
    });
    while (m.Count()) {
        std::cout << "cursor = " << m.cursor << std::endl;
        m.Update();
    }
	return 0;
}


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
