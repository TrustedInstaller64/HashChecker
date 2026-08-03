#include "hash/parallel_computer.h"

#include "hash/hasher_registry.h"
#include "infra/thread_pool.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace hash {

ParallelComputer::ParallelComputer(infra::ThreadPool& pool, const std::string& algorithm,
                                   std::size_t buffer_size)
    : pool_(pool), algorithm_(algorithm), buffer_size_(buffer_size) {}

std::vector<std::future<HashTask>>
ParallelComputer::submit_all(const std::vector<scan::ScanEntry>& entries,
                             const fs::path& root) {
    std::vector<std::future<HashTask>> futures;
    futures.reserve(entries.size());

    for (const auto& entry : entries) {
        auto full_path = root / entry.relative_path;
        auto rel_path_str = entry.relative_path.string();
        std::string algo = algorithm_;
        std::size_t buf_size = buffer_size_;

        futures.push_back(pool_.submit([full_path, rel_path_str, algo, buf_size]() -> HashTask {
            HashTask task;
            task.relative_path = rel_path_str;

            auto hasher = HasherRegistry::instance().create(algo);
            if (!hasher) {
                task.error = "Unknown algorithm: " + algo;
                return task;
            }

            std::ifstream file(full_path, std::ios::binary);
            if (!file) {
                task.error = "Cannot open file";
                return task;
            }

            std::vector<uint8_t> buffer(buf_size);
            try {
                while (file.read(reinterpret_cast<char*>(buffer.data()),
                                 static_cast<std::streamsize>(buf_size)) ||
                       file.gcount() > 0) {
                    hasher->update(buffer.data(),
                                   static_cast<std::size_t>(file.gcount()));
                }
            } catch (const std::exception& e) {
                task.error = std::string("Read error: ") + e.what();
                return task;
            }

            hasher->finalize();
            task.hash_value = hasher->hexdigest();
            return task;
        }));
    }

    return futures;
}

} // namespace hash
