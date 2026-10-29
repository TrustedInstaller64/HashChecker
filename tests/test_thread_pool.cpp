#include "infra/thread_pool.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

TEST_CASE("ThreadPool: submit and wait_all") {
    infra::ThreadPool pool(4);

    std::atomic<int> counter{0};
    constexpr int kTasks = 100;

    std::vector<std::future<void>> futures;
    for (int i = 0; i < kTasks; ++i) {
        futures.push_back(pool.submit([&counter]() { ++counter; }));
    }

    pool.wait_all();

    // All futures should be ready
    for (auto& f : futures) {
        CHECK_NOTHROW(f.get());
    }

    CHECK(counter == kTasks);
}

TEST_CASE("ThreadPool: submit returns correct values") {
    infra::ThreadPool pool(4);

    auto f1 = pool.submit([]() { return 42; });
    auto f2 = pool.submit([]() { return std::string("hello"); });

    CHECK(f1.get() == 42);
    CHECK(f2.get() == "hello");
}

TEST_CASE("ThreadPool: exception propagation") {
    infra::ThreadPool pool(2);

    auto f = pool.submit([]() -> int { throw std::runtime_error("test error"); });

    CHECK_THROWS_AS(f.get(), std::runtime_error);
}

TEST_CASE("ThreadPool: worker_count") {
    infra::ThreadPool pool(3);
    CHECK(pool.worker_count() == 3);
}

TEST_CASE("ThreadPool: pool lifetime with no tasks") {
    // Should not hang or crash
    { infra::ThreadPool pool(4); }
    CHECK(true);
}
