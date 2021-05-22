#include <coroutine>
#include <exception>
#include <iostream>
#include <tuple>
#include "xx_coro.h"
#include "xx_nodepool.h"

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

    // int: wheel index
    xx::NodePool<std::tuple<int, xx::Generator<Cond>>> nodes;

    int wheelLen;
    int *wheel;
    int cursor = 0;
    double frameDelaySeconds;

    explicit Coros(double const &framePerSeconds = 10, int const &wheelLen = 10 * 60 * 5)
            : frameDelaySeconds(1.0 / framePerSeconds), wheelLen(wheelLen) {
        wheel = (int *) malloc(wheelLen * sizeof(int));
        memset(wheel, -1, wheelLen * sizeof(int));
    }

    ~Coros() {
        free(wheel);
    }

    void Clear() {
        nodes.Clear();
        memset(wheel, -1, wheelLen * sizeof(int));
        cursor = 0;
    }

protected:

    void WheelAdd(int const &idx, int const &wIdx) {
        auto& n = nodes[idx];
        n.prev = -1;
        n.next = wheel[wIdx];
        std::get<0>(n.value) = wIdx;
        if (wheel[wIdx] >= 0) {
            nodes[wheel[wIdx]].prev = idx;
        }
        wheel[wIdx] = idx;
    }

    // todo: event occur: stop sleep
    void WheelRemove(int const &idx) {
        auto& n = nodes[idx];
        assert(n.prev != -2);
        auto& wIdx = std::get<0>(n.value);
        assert(wIdx >= 0);
        if (n.prev < 0 && wheel[wIdx] == idx) {
            wheel[wIdx] = n.next;
        } else {
            nodes[n.prev].next = n.next;
        }
        if (n.next >= 0) {
            nodes[n.next].prev = n.prev;
        }
        wIdx = -1;
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
        return nodes.Count() > 0;
    }

    // c: Coro Func(...) { ... co_yield Cond().xxxx; ... co_return
    void Add(xx::Generator<Cond> &&g) {
        if (g.Done()) return;
        auto&& c = g.Value();
        auto n = CalcSleepTimes(c);
        auto idx = nodes.Alloc(0, std::move(g));
        auto wIdx = (cursor + n) % wheelLen;
        WheelAdd(idx, wIdx);
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
            auto& n = nodes[idx];
            auto next = n.next;
            auto& coro = std::get<1>(n.value);
            if (coro.Resume()) {
                nodes.Free(idx);
            } else {
                auto&& cond = coro.Value();
                auto num = CalcSleepTimes(cond);
                auto wIdx = (cursor + num) % wheelLen;
                WheelAdd(idx, wIdx);
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
    //auto w = SharedFromThis(this).ToWeak();
    CoAwait(Test());
    //if(auto self = w.Lock()) self->
    //this->Send

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

