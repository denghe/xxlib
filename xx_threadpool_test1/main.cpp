#include "xx_threadpool.h"
#include "xx_ptr.h"
#include <iostream>

// 模拟主线程网络逻辑，线程池数据逻辑的典型应用

// 模拟主线程上下文
struct Env {
    std::mutex actionsMutex;
    std::deque<std::function<void()>> actions;

    template<typename F>
    inline void Dispatch(F &&f) {
        std::scoped_lock<std::mutex> sl(actionsMutex);
        actions.emplace_back(std::forward<F>(f));
    }

    inline int HandleActions() {
        while (true) {
            // 当前正要执行的函数
            std::function < void() > action;
            {
                std::scoped_lock<std::mutex> g(actionsMutex);
                if (actions.empty()) break;
                action = std::move(actions.front());
                actions.pop_front();
            }
            action();
        }
        return 0;
    }

    bool running = true;
    inline void Run() {
        while (running) {
            HandleActions();
            Sleep(100);
        }
    }

    // 共享：加持 & 封送
    template<typename T>
    xx::SharedBox<T> ToSharedBox(xx::Shared<T> const& s) {
        return xx::SharedBox<T>(s, [this](T **p) { Dispatch([p] { xx::Shared<T> o; o.pointer = *p; }); });
    }
    // 如果独占：不加持 不封送 就地删除
    template<typename T>
    xx::SharedBox<T> ToSharedBox(xx::Shared<T> && s) {
        if (s.GetHeader()->sharedCount == 1) return xx::SharedBox<T>(std::move(s), [this](T **p) { xx::Shared<T> o; o.pointer = *p; });
        else return xx::SharedBox<T>(s, [this](T **p) { Dispatch([p] { xx::Shared<T> o; o.pointer = *p; }); });
    }
};

// 模拟主线上下文 & 线程池 实例
inline Env env;
inline xx::ThreadPool<> tp;

// 模拟业务逻辑
struct Foo {
    int id = 0;
    explicit Foo(int id) : id(id) {
        std::cout << id << " new foo at " << std::this_thread::get_id() << std::endl;
    }
    ~Foo() {
        std::cout << id << " delete foo at " << std::this_thread::get_id() << std::endl;
    }
};

int main() {
    // 模拟主线逻辑 得到数据, 压到线程池搞事情
    {
        auto foo = xx::Make<Foo>(1);
        tp.Add([foo = env.ToSharedBox(foo)] {
            std::cout << foo->id << std::endl;
        });
    }
    {
        auto foo = xx::Make<Foo>(2);
        tp.Add([foo = env.ToSharedBox(std::move(foo))] {
            std::cout << foo->id << std::endl;
        });
    }
    env.Run();

    std::cout << "end." << std::endl;
    return 0;
}
