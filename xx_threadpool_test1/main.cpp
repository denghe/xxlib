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

    template<typename T>
    xx::Ptr<T> ToPtr(xx::Shared<T> &s) {
        return s.ToPtr([this](T **p) { Dispatch([p] { xx::Shared<T> o; o.pointer = *p; }); });
    }
};

struct Foo {
    int id = 0;
    std::string name = "asdf";

    Foo() {
        std::cout << "new foo at " << std::this_thread::get_id() << std::endl;
    }

    ~Foo() {
        std::cout << "delete foo at " << std::this_thread::get_id() << std::endl;
    }
};

int main() {
    Env env;
    xx::ThreadPool<> tp;

    // 模拟主线逻辑 得到数据, 压到线程池搞事情
    {
        auto foo = xx::Make<Foo>();

        tp.Add([foo = env.ToPtr(foo)] {
            std::cout << foo->id << " " << foo->name << std::endl;
        });
    }

    // 模拟主线循环
    while (true) {
        env.HandleActions();
        Sleep(100);
    }

//	for (size_t i = 0; i < 100; i++)
//	{
//		tp.Add([] {std::cout << std::this_thread::get_id() << std::endl; });
//	}
    std::cout << "end." << std::endl;
    return 0;
}
