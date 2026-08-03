#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace infra {

/// A fixed-size thread pool that executes submitted tasks concurrently.
class ThreadPool {
public:
    /// Create a thread pool with `num_threads` workers.
    /// If num_threads == 0, uses std::thread::hardware_concurrency().
    explicit ThreadPool(std::size_t num_threads);

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// Submit a callable and return a future for its result.
    /// If the pool has been stopped, throws std::runtime_error.
    template <typename F>
    auto submit(F&& task) -> std::future<decltype(task())>;

    /// Block until all currently submitted tasks have completed.
    void wait_all();

    /// Return the number of worker threads.
    std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    void worker_loop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::condition_variable finished_condition_;
    std::atomic<std::size_t> active_tasks_{0};
    bool stop_{false};
};

// ── Template implementation ─────────────────────────────

template <typename F>
auto ThreadPool::submit(F&& task) -> std::future<decltype(task())> {
    using result_type = decltype(task());

    auto packaged = std::make_shared<std::packaged_task<result_type()>>(
        std::forward<F>(task));
    auto future = packaged->get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool: cannot submit to a stopped pool");
        }
        ++active_tasks_;
        tasks_.emplace([packaged]() { (*packaged)(); });
    }

    condition_.notify_one();
    return future;
}

} // namespace infra
