#pragma once
#include <xx_includes.h>

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
    template<typename Env, size_t numThreads, typename Job = std::function<void(Env&)>>
    struct ThreadPool2 {
        std::array<Env, numThreads> envs;
    protected:
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
                        e(j);
                    }
                    });
            }
        }

        template<typename...Args>
        int Add(Args&&... args) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::forward<Args>(args)...);
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

        // for RunOnce
        void Process(int const& tid) {
            auto& fs = funcss[tid];
            // 执行所有函数
            for (auto& f : fs) f();
            fs.clear();
        }

        // For Begin + Add + Commit
        std::atomic<bool> running;
        void Process2(int const& tid) {
            while (!running) std::this_thread::sleep_for(0s);
            Process(tid);
        }

        // For Start + Add + Go + Add + Go + Add + Go.... + Stop
        std::atomic<bool> started;
        std::atomic<int> numFinisheds;
        void Process3(int const& tid) {
            do {
                while (started && !running) std::this_thread::sleep_for(0s);
                if (!started) return;
                Process(tid);
                ++numFinisheds;
                while (numFinisheds) std::this_thread::sleep_for(0s);
            } while (started);
        }
    public:
        // 初始化线程个数
        void Init(int const& numThreads) {
            threads.resize(numThreads);
            funcss.clear();
            funcss.resize(numThreads);
        }

        /****************************************************************************************/
        /****************************************************************************************/

        // 往指定线程的任务队列压入函数
        template<typename F>
        void AddTo(int const& tid, F&& func) {
            assert(!threads.empty());
            funcss[tid].emplace_back(std::forward<F>(func));
        }

        // 找个数少的任务队列压入函数( 推荐直接指定 tid )
        template<typename F>
        void Add(F&& func) {
            assert(!threads.empty());
            int n = (int)threads.size();
            int idx = 0;
            auto m = funcss[idx].size();
            for (int i = 1; i < n; ++i) {
                if (auto c = funcss[i].size(); c < m) {
                    m = c;
                    idx = i;
                }
            }
            AddTo(idx, std::forward<F>(func));
        }

        /****************************************************************************************/
        /****************************************************************************************/


        // 一起执行并阻塞等待执行完毕，任务会清空. 无多余 cpu 消耗, 
        void RunOnce() {
            assert(!threads.empty());
            int n = (int)threads.size();
            for (int i = 0; i < n; ++i) {
                threads[i] = std::thread([this, i = i] { Process(i); });
            }
            for (std::thread& t : threads) { t.join(); }
        }

        /****************************************************************************************/
        /****************************************************************************************/

        // 线程开始执行 但是 卡 running，会剧烈消耗 CPU. 在 Commit 之前，可 Add job 或别的轻微耗时逻辑
        void Begin() {
            assert(!threads.empty());
            assert(!running);
            int n = (int)threads.size();
            for (int i = 0; i < n; ++i) {
                threads[i] = std::thread([this, i = i] { Process2(i); });
            }
        }

        // running = true 一起执行并阻塞等待执行完毕，任务会清空
        void Commit() {
            assert(!threads.empty());
            assert(!running);
            running = true;
            for (std::thread& t : threads) { t.join(); }
            running = false;
        }

        /****************************************************************************************/
        /****************************************************************************************/

        // 线程开始执行 但是 卡 running，会剧烈消耗 CPU. 配套执行函数是 Go
        void Start() {
            assert(!threads.empty());
            assert(!started && !running);
            started = true;
            int n = (int)threads.size();
            for (int i = 0; i < n; ++i) {
                threads[i] = std::thread([this, i = i] { Process3(i); });
            }
        }

        // 执行一把 Add 的函数。线程保持运行，并吃 CPU
        void Go() {
            assert(!threads.empty());
            assert(started && !running);
            running = true;
            int n = (int)threads.size();
            while (numFinisheds != n) {
                std::this_thread::sleep_for(0s);
            }
            running = false;
            numFinisheds = 0;
        }

        // 线程停止执行
        void Stop() {
            assert(!threads.empty());
            assert(started && !running);
            started = false;
            for (std::thread& t : threads) { t.join(); }
        }

        ~ToggleThreadPool() {
            if (started) Stop();
        }
    };
}
