#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace xx {

    // 常规线程池
    template<typename JT = std::function<void()>>
    class ThreadPool {
        std::vector<std::thread> threads;
        std::queue<JT> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

    public:
        explicit ThreadPool(int const& numThreads = 4) {
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([this] {
                    while (true) {
                        JT job;
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            this->cond.wait(lock, [this] {
                                return this->stop || !this->jobs.empty();
                                });
                            if (this->stop && this->jobs.empty()) return;
                            job = std::move(this->jobs.front());
                            this->jobs.pop();
                        }
                        job();
                    }
                    });
            }
        }

        int Add(JT&& job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::move(job));
            }
            cond.notify_one();
            return 0;
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
    };


    // 带执行环境的版本
    template<typename Env, size_t numThreads>
    struct ThreadPool2 {
        std::vector<Env> envs;
    protected:
        using JT = std::function<void(Env&)>;
        std::vector<std::thread> threads;
        std::queue<JT> jobs;
        std::mutex mtx;
        std::condition_variable cond;
        bool stop = false;

    public:
        explicit ThreadPool2() {
            envs.resize(numThreads);
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back([this, i = i] {
                    auto& t = envs[i];
                    while (true) {
                        JT job;
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            this->cond.wait(lock, [this] {
                                return this->stop || !this->jobs.empty();
                                });
                            if (this->stop && this->jobs.empty()) return;
                            job = std::move(this->jobs.front());
                            this->jobs.pop();
                        }
                        t(job);
                    }
                    });
            }
        }

        int Add(JT&& job) {
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop) return -1;
                jobs.emplace(std::move(job));
            }
            cond.notify_one();
            return 0;
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
    };

    /*
    struct Env {
        // ...

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
}
