#pragma once

#include <vector>
#include <array>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
using namespace std::chrono_literals;

namespace xx {

    // 常规线程池
    template<typename Job = std::function<void()>>
    class ThreadPool {
        std::vector<std::thread> threads;
        std::queue<Job> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

    public:
        explicit ThreadPool(int const& numThreads = 4) {
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([this] {
                    while (true) {
                        Job j;
                        {
                            std::unique_lock<std::mutex> lock(mtx);
                            cond.wait(lock, [this] {
                                return stop || !jobs.empty();
                                });
                            if (stop && jobs.empty()) return;
                            j = std::move(jobs.front());
                            jobs.pop();
                        }
                        j();
                    }
                    });
            }
        }

        template<typename Func>
        int Add(Func&& job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::forward<Func>(job));
            }
            cond.notify_one();
            return 0;
        }

        operator bool() {
            std::unique_lock<std::mutex> lock(mtx);
            return !jobs.empty();
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(mtx);
                stop = true;
            }
            cond.notify_all();
            for (std::thread& worker : threads) {
                worker.join();
            }
        }

        void Join() {
            while(*this) {
                std::this_thread::sleep_for(100ms);
            }
        }
    };




    // SFINAE test 检查目标类型是否带有 operator() 函数
    template <typename T>
    class has_OperatorParentheses {
        typedef char one;
        struct two { char x[2]; };

        template <typename C> static one test(decltype(&C::operator()));
        template <typename C> static two test(...);

    public:
        enum { value = sizeof(test<T>(0)) == sizeof(char) };
    };


    // 带执行环境的版本
    template<typename Env, size_t numThreads>
    struct ThreadPool2 {
        std::array<Env, numThreads> envs;
    protected:
        using Job = std::function<void(Env&)>;
        std::array<std::thread, numThreads> threads;
        std::queue<Job> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

    public:
        explicit ThreadPool2() {
            for (int i = 0; i < numThreads; ++i) {
                threads[i] = std::thread([this, i = i] {
                    auto& e = envs[i];
                    while (true) {
                        Job j;
                        {
                            std::unique_lock<std::mutex> lock(mtx);
                            cond.wait(lock, [this] {
                                return stop || !jobs.empty();
                                });
                            if (stop && jobs.empty()) return;
                            j = std::move(jobs.front());
                            jobs.pop();
                        }
                        // 检测 Env 是否存在 operator(). 如果没有这个函数，就执行 j(e);
                        if constexpr (has_OperatorParentheses<Env>::value) {
                            e(j);
                        }
                        else {
                            j(e);
                        }
                    }
                    });
            }
        }

        template<typename Func>
        int Add(Func&& job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::forward<Func>(job));
            }
            cond.notify_one();
            return 0;
        }

        operator bool() {
            std::unique_lock<std::mutex> lock(mtx);
            return !jobs.empty();
        }

        ~ThreadPool2() {
            {
                std::unique_lock<std::mutex> lock(mtx);
                stop = true;
            }
            cond.notify_all();
            for (std::thread& worker : threads) {
                worker.join();
            }
        }

        void Join() {
            while(*this) {
                std::this_thread::sleep_for(100ms);
            }
        }
    };


    


    /*
    // 池里的线程的公用环境
    struct Env {
        // ...

        // 如果提供了这个函数，就会被调用。可以写一些重复出现的代码在此
        void operator()(std::function<void(Env&)>& job) {
            try {
                job(*this);
            }
            catch (int const& e) {
                xx::CoutN("catch throw int: ", e);
            }
            catch (std::exception const& e) {
                xx::CoutN("catch throw std::exception: ", e);
            }
            catch (...) {
                xx::CoutN("catch ...");
            }
        }
    };

    xx::ThreadPool2<Env, 4> tp;

    std::array<xx::ThreadPool2<Env, 1>, 4> tps;

    tp.Add([](Env& e) {
        // e.xxxxx
    });

    */







    // 翻转执行的线程池。只有当 RunOnce 的时候，每个线程才开始执行。RunOnce 会阻塞到所有线程执行完毕。
    class ToggleThreadPool {
        // 线程池
        std::vector<std::thread> threads;
        // 每个线程对应的当次执行队列
        std::vector<std::vector<std::function<void()>>> funcss;

        void Process(int const& tid) {
            auto& fs = funcss[tid];
            // 执行所有函数
            for (auto& f : fs) f();
            fs.clear();
        }
    public:
        // 初始化线程个数
        void Init(int const& numThreads) {
            threads.resize(numThreads);
            funcss.resize(numThreads);
        }
        // 往指定线程的执行队列压入函数
        template<typename F>
        void Add(int const& tid, F&& func) {
            funcss[tid].emplace_back(std::forward<F>(func));
        }
        // 一起执行
        void RunOnce() {
            int n = (int)threads.size();
            for (int i = 0; i < n; ++i) {
                threads[i] = std::thread([this, i = i] { Process(i); });
            }
            for (std::thread& t : threads) { t.join(); }
        }
    };

}
