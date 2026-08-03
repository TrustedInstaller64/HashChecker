#include "infra/thread_pool.h"

#include <cstdio>

#ifdef __APPLE__
#include <pthread.h>
#endif

namespace infra {

ThreadPool::ThreadPool(std::size_t num_threads) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4; // fallback
        }
        // Cap at 80% — avoids thrashing on I/O-bound network drives
        num_threads = std::max(std::size_t(1), num_threads * 4 / 5);
    }

    workers_.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this, i]() {
#ifdef __APPLE__
            char name[32];
            std::snprintf(name, sizeof(name), "hc-worker-%zu", i);
            pthread_setname_np(name);
#endif
            worker_loop();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            --active_tasks_;
        }
        finished_condition_.notify_all();
    }
}

void ThreadPool::wait_all() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    finished_condition_.wait(lock, [this] { return active_tasks_ == 0; });
}

} // namespace infra
