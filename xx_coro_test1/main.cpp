#include <coroutine>
#include <exception>
#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_set>
#include <cassert>
#include <cstring>
#include "xx_coro.h"

// co_yield Cond().Xxxxx(v).Xxxx(v)...;
struct Cond {
    Cond(Cond const &) = default;

    Cond &operator=(Cond const &) = default;

    Cond(Cond &&) = default;

    Cond &operator=(Cond &&) = default;

    union {
        uint32_t flags = 0;
        struct {
            uint8_t hasSleepTimes;
            uint8_t hasSleepSeconds;
            //uint8_t hasWaitTypeId;
            //uint8_t hasWaitSerial;
            // more
        };
    };

    int sleepTimes = 0;
    double sleepSeconds = 0.0;
//    int serial = 0;
//    uint16_t typeId = 0;
    // more

    Cond() = default;

    Cond(int const &sleepTimes_) {
        SleepTimes(sleepTimes_);
    }

    Cond(double const &sleepSeconds_) {
        SleepSeconds(sleepSeconds_);
    }

    Cond &&SleepTimes(int const &v) {
        hasSleepTimes = true;
        sleepTimes = v;
        return std::move(*this);
    }

    Cond &&SleepSeconds(double const &v) {
        hasSleepSeconds = true;
        sleepSeconds = v;
        return std::move(*this);
    }

    // more
};

struct Coros {
    Coros(Coros const &) = delete;

    Coros &operator=(Coros const &) = delete;

    Coros(Coros &&) = default;

    Coros &operator=(Coros &&) = default;

    /*************************************************/
    struct Node {
        xx::Generator<Cond> g;
        int prev, next, wIdx;    // for wheel
        // todo: more cond idx here for remove sync
    };
    /*************************************************/
    int freeList = -1;
    int freeCount = 0;
    int count = 0;
    size_t cap;
    Node *nodes;
    /*************************************************/
    int wheelLen;
    int *wheel;
    int cursor = 0;
    double frameDelaySeconds;

    explicit Coros(size_t const &cap = 8192, double const &framePerSeconds = 10, int const &wheelLen = 10 * 60 * 5)
            : cap(cap), frameDelaySeconds(1.0 / framePerSeconds), wheelLen(wheelLen) {
        assert(cap > 0);
        nodes = (Node *) malloc(cap * sizeof(Node));
        wheel = (int *) malloc(wheelLen * sizeof(int));
        memset(wheel, -1, wheelLen * sizeof(int));
    }

    ~Coros() {
        Clear();
        free(wheel);
        free(nodes);
    }

    void Clear() {
        for (int i = 0; i < count; ++i) {
            if (nodes[i].prev == -2) continue;
            nodes[i].g.~Generator<Cond>();
        }
        freeList = -1;
        freeCount = 0;
        count = 0;
        memset(wheel, -1, wheelLen * sizeof(int));
        cursor = 0;
    }

protected:
    int Alloc() {
        int idx;
        if (freeCount > 0) {
            idx = freeList;
            freeList = nodes[idx].next;
            freeCount--;
        } else {
            if (count == cap) {
                cap *= 2;
                nodes = (Node *) realloc(nodes, cap);
            }
            idx = count;
            count++;
        }
        return idx;
    }

    void Free(int const &idx) {
        assert(idx >= 0 && idx < count && nodes[idx].prev != -2);
        nodes[idx].next = freeList;
        freeList = idx;
        freeCount++;
        nodes[idx].prev = -2;           // -2: foreach 时的无效标志
    }

    void AddToWheel(int const &idx, int const &wIdx) {
        nodes[idx].prev = -1;
        nodes[idx].next = wheel[wIdx];
        nodes[idx].wIdx = wIdx;
        if (wheel[wIdx] >= 0) {
            nodes[wheel[wIdx]].prev = idx;
        }
        wheel[wIdx] = idx;
    }

    void RemoveFromWheel(int const &idx) {
        assert(nodes[idx].wIdx >= 0);
        if (nodes[idx].prev < 0 && wheel[nodes[idx].wIdx] == idx) {
            wheel[nodes[idx].wIdx] = nodes[idx].next;
        } else {
            nodes[nodes[idx].prev].next = nodes[idx].next;
        }
        if (nodes[idx].next >= 0) {
            nodes[nodes[idx].next].prev = nodes[idx].prev;
        }
        nodes[idx].wIdx = -1;
    }


    int CalcSleepTimes(Cond const &c) const {
        int n;
        if (c.hasSleepTimes) {
            assert(c.sleepTimes > 0);
            n = c.sleepTimes;
        } else if (c.hasSleepSeconds) {
            assert(c.sleepSeconds > 0);
            n = (int) (c.sleepSeconds / frameDelaySeconds);
            if (n == 0) {
                n == 1;
            }
        } else {
            n = 1;
        }
        assert(n < wheelLen);
        return n;
    }

public:

    operator bool() const {
        return count - freeCount > 0;
    }

    // c: Coro Func(...) { ... co_yield Cond().xxxx; ... co_return
    void Add(xx::Generator<Cond> &&g) {
        if (g.Done()) return;
        auto&& c = g.Value();
        auto n = CalcSleepTimes(c);
        auto wIdx = (cursor + n) % wheelLen;
        auto idx = Alloc();
        AddToWheel(idx, wIdx);
        auto &node = nodes[idx];
        new (&node.g) xx::Generator<Cond>(std::move(g));
        // todo: handle other conds
    }

    // update time wheel, resume list coros
    void operator()() {
        cursor = (cursor + 1) % ((int) wheelLen - 1);
        if (wheel[cursor] == -1) return;
        auto idx = wheel[cursor];
        assert(nodes[idx].prev == -1);
        wheel[cursor] = -1;
        do {
            auto next = nodes[idx].next;
            if (nodes[idx].g.Resume()) {
                nodes[idx].g.~Generator<Cond>();
                Free(idx);
            } else {
                auto&& c = nodes[idx].g.Value();
                auto n = CalcSleepTimes(c);
                auto wIdx = (cursor + n) % wheelLen;
                AddToWheel(idx, wIdx);
            }
            idx = next;
        } while (idx != -1);
    }
};

xx::Generator<Cond> Test() {
    for (int i = 3; i <= 4; ++i) {
        std::cout << i << std::endl;
        co_yield i;
    }
    std::cout << "End" << std::endl;
}

xx::Generator<Cond> Test2() {
    CoAwait(Test());
//    auto&& g = Test();
//    while(!g.Resume()) {
//        co_yield g.Value();
//    }
}


int main() {
    Coros cs;
    cs.Add(Test2());
    while (cs) {
        std::cout << "------------------- " << cs.cursor << " -------------------" << std::endl;
        cs();
    }
    return 0;
}


//
//CoRtv Delay(int n) {
//    while(n--) CoYield;
//}
//
//CoRtv Test(int id) {
//    CoAwait(Delay(1));
//    std::cout << id << std::endl;
//    CoAwait(Delay(2));
//    std::cout << id << std::endl;
//}
//
//int main() {
//    xx::Coros coros;
//    //coros.Add()
//	return 0;
//}

