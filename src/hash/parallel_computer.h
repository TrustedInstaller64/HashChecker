#pragma once

#include "scan/file_scanner.h"
#include <cstddef>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

namespace infra {
class ThreadPool;
} // namespace infra

namespace hash {

/// Holds the result of hashing a single file.
struct HashTask {
    std::string relative_path;
    std::string hash_value;
    std::string error; // empty on success
};

/// Schedules parallel hash computations using a ThreadPool.
class ParallelComputer {
public:
    /// @param pool     Thread pool to schedule work on.
    /// @param algorithm Hash algorithm name (e.g. "sha256").
    /// @param buffer_size Read buffer size in bytes (default 64 KiB).
    ParallelComputer(infra::ThreadPool& pool, const std::string& algorithm,
                     std::size_t buffer_size = 65536);

    /// Submit all entries for hashing. Returns one future per entry.
    /// Each future resolves to a HashTask with the hex digest or error.
    std::vector<std::future<HashTask>>
    submit_all(const std::vector<scan::ScanEntry>& entries,
               const std::filesystem::path& root);

private:
    infra::ThreadPool& pool_;
    std::string algorithm_;
    std::size_t buffer_size_;
};

} // namespace hash
