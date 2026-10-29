#include "app.h"

#include "compare/comparator.h"
#include "hash/hash_task.h"
#include "hash/hasher_registry.h"
#include "hash/register_all.h"
#include "infra/thread_pool.h"
#include "report/json_reporter.h"
#include "report/quiet_reporter.h"
#include "report/text_reporter.h"
#include "scan/file_scanner.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <thread>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::unique_ptr<report::IReporter> create_reporter(const std::string& format) {
    if (format == "quiet") {
        return std::make_unique<report::QuietReporter>();
    }
    if (format == "json") {
        return std::make_unique<report::JsonReporter>();
    }
    return std::make_unique<report::TextReporter>();
}

void save_hash_log(const std::map<std::string, hash::HashTask>& task_map,
                   const fs::path& file_path) {
    std::ofstream out(file_path);
    if (!out) {
        std::cerr << "Warning: cannot save log to " << file_path << '\n';
        return;
    }
    for (const auto& [path, task] : task_map) {
        out << path << " | " << (task.error.empty() ? task.hash_value : "ERROR: " + task.error)
            << '\n';
    }
}

// Hash `path` in 64 KiB chunks and store the lowercase hex digest in `hash`.
// On failure returns false, sets `error`, and leaves `hash` untouched.
bool hash_file(const fs::path& path, const std::string& algorithm, std::string& hash,
               std::string& error) {
    auto hasher = hash::HasherRegistry::instance().create(algorithm);
    if (!hasher) {
        error = "Unknown algorithm: " + algorithm;
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Cannot open file";
        return false;
    }

    constexpr std::size_t kBufSize = 65536;
    std::vector<uint8_t> buffer(kBufSize);
    try {
        while (file.read(reinterpret_cast<char*>(buffer.data()), kBufSize) ||
               file.gcount() > 0) {
            hasher->update(buffer.data(), static_cast<std::size_t>(file.gcount()));
        }
    } catch (const std::exception& e) {
        error = std::string("Read error: ") + e.what();
        return false;
    }

    hasher->finalize();
    hash = hasher->hexdigest();
    return true;
}

} // namespace

int run(const AppConfig& config) {
    // Convert CLI strings to proper Unicode paths (u8path avoids code-page issues)
    fs::path target_path = fs::u8path(config.target_dir);
    fs::path ref_path = fs::u8path(config.reference_dir);

    // ── 1. Validate paths ─────────────────────────────────
    if (!fs::exists(target_path)) {
        std::cerr << "Error: target not found: " << target_path.u8string() << '\n';
        return 2;
    }
    if (!fs::exists(ref_path)) {
        std::cerr << "Error: reference not found: " << ref_path.u8string() << '\n';
        return 2;
    }

    bool target_is_dir = fs::is_directory(target_path);
    bool ref_is_dir = fs::is_directory(ref_path);

    if (target_is_dir != ref_is_dir) {
        std::cerr << "Error: cannot compare a file with a directory.\n";
        return 2;
    }

    bool single_file = !target_is_dir;

    // ── 2. Validate algorithm ────────────────────────────
    auto test_hasher = hash::HasherRegistry::instance().create(config.algorithm);
    if (!test_hasher) {
        std::cerr << "Error: unknown algorithm '" << config.algorithm << "'\n";
        auto available = hash::HasherRegistry::instance().available_algorithms();
        if (!available.empty()) {
            std::cerr << "Available:";
            for (const auto& a : available) {
                std::cerr << ' ' << a;
            }
            std::cerr << '\n';
        }
        return 2;
    }

    std::string report_format = config.report_format;

    // ── 3a. Single-file mode ──────────────────────────────
    if (single_file) {
        std::string target_hash, ref_hash, target_err, ref_err;
        if (!config.quiet) {
            std::cout << "Computing hash of " << target_path.filename().u8string() << " ...\n";
            std::cout << "Computing hash of " << ref_path.filename().u8string() << " ...\n";
        }

        // The two files are independent and hashing is CPU-bound, so hash them
        // concurrently. On a single large-file pair this roughly halves the time.
        auto target_future = std::async(std::launch::async, [&] {
            return hash_file(target_path, config.algorithm, target_hash, target_err);
        });
        auto ref_future = std::async(std::launch::async, [&] {
            return hash_file(ref_path, config.algorithm, ref_hash, ref_err);
        });
        // Errors (if any) are reported through target_err/ref_err below.
        (void)target_future.get();
        (void)ref_future.get();

        if (!target_err.empty()) {
            std::cerr << "Error (target): " << target_err << '\n';
            return 2;
        }
        if (!ref_err.empty()) {
            std::cerr << "Error (reference): " << ref_err << '\n';
            return 2;
        }

        compare::ComparisonResult result;
        result.algorithm = config.algorithm;
        result.target_root = target_path.u8string();
        result.reference_root = ref_path.u8string();

        if (target_hash == ref_hash) {
            result.matched = 1;
        } else {
            result.hash_mismatch = 1;
            compare::DiffEntry diff;
            diff.relative_path = target_path.filename().u8string();
            diff.target_hash = target_hash;
            diff.reference_hash = ref_hash;
            diff.kind = compare::DiffKind::kHashMismatch;
            result.differences.push_back(std::move(diff));
        }

        auto reporter = create_reporter(report_format);
        if (config.output_file.empty()) {
            reporter->report(result, std::cout);
        } else {
            std::ofstream out(config.output_file);
            if (!out) { std::cerr << "Error: cannot write to " << config.output_file << '\n'; return 2; }
            reporter->report(result, out);
        }
        return result.identical() ? 0 : 1;
    }

    // ── 3b. Create thread pools (directory mode) ──────────
    std::size_t target_th = config.target_threads;
    std::size_t ref_th = config.reference_threads;

    if (target_th == 0 && ref_th == 0 && config.num_threads == 0) {
        // Auto: 40% each, 80% total
        std::size_t hw = std::thread::hardware_concurrency();
        if (hw == 0) hw = 4;
        target_th = std::max(std::size_t(1), hw * 2 / 5);
        ref_th = std::max(std::size_t(1), hw * 2 / 5);
    } else {
        // -j = total threads, split evenly between both drives
        std::size_t half = std::max(std::size_t(1), config.num_threads / 2);
        if (target_th == 0) target_th = half;
        if (ref_th == 0) ref_th = half;
    }

    infra::ThreadPool target_pool(target_th);
    infra::ThreadPool ref_pool(ref_th);

    if (!config.quiet) {
        std::cout << "HashChecker v1.0.0\n"
                  << "Algorithm: " << config.algorithm << '\n'
                  << "Target threads:    " << target_pool.worker_count() << '\n'
                  << "Reference threads: " << ref_pool.worker_count() << '\n'
                  << "Target:    " << target_path.u8string() << '\n'
                  << "Reference: " << ref_path.u8string() << '\n'
                  << '\n';
    }

    // ── 4. Scan both directories in parallel ─────────────
    if (!config.quiet) {
        std::cout << "Scanning directories...\n";
    }

    scan::ScanOutcome target_scan;
    scan::ScanOutcome ref_scan;

    try {
        auto target_scan_future =
            std::async(std::launch::async,
                       [&]() { return scan::FileScanner::scan(target_path); });
        auto ref_scan_future =
            std::async(std::launch::async,
                       [&]() { return scan::FileScanner::scan(ref_path); });

        target_scan = target_scan_future.get();
        ref_scan = ref_scan_future.get();
    } catch (const std::exception& e) {
        std::cerr << "Error during scan: " << e.what() << '\n';
        return 2;
    } catch (...) {
        std::cerr << "Error during scan: unknown exception\n";
        return 2;
    }

    // Report scan errors
    for (const auto& err : target_scan.errors) {
        if (!config.quiet) {
            std::cerr << "Warning (target): " << err.relative_path << " - " << err.message << '\n';
        }
    }
    for (const auto& err : ref_scan.errors) {
        if (!config.quiet) {
            std::cerr << "Warning (reference): " << err.relative_path << " - " << err.message
                      << '\n';
        }
    }

    if (!config.quiet) {
        std::cout << "Target:    " << target_scan.entries.size() << " files ("
                  << target_scan.total_bytes << " bytes)\n";
        std::cout << "Reference: " << ref_scan.entries.size() << " files ("
                  << ref_scan.total_bytes << " bytes)\n\n";
    }

    // ── 5. Hash files in parallel via thread pool ────────
    if (!config.quiet) {
        std::cout << "Computing hashes...\n" << std::flush;
    }

    // Per-drive progress counters
    std::atomic<std::size_t> t_done{0};
    std::atomic<std::size_t> r_done{0};
    std::size_t t_total = target_scan.entries.size();
    std::size_t r_total = ref_scan.entries.size();

    // Wrapper that updates progress counter
    auto submit_with_progress = [&](const std::vector<scan::ScanEntry>& entries,
                                    const fs::path& root, infra::ThreadPool& pool,
                                    const char* label)
        -> std::vector<std::future<hash::HashTask>> {
        std::vector<std::future<hash::HashTask>> futures;
        futures.reserve(entries.size());

        for (const auto& entry : entries) {
            auto full_path = root / entry.relative_path;
            auto rel_path_str = entry.relative_path.u8string();
            std::string algo = config.algorithm;

            futures.push_back(pool.submit([full_path, rel_path_str, algo,
                                           verbose = config.verbose, label,
                                           counter = (label[0] == 'T' ? &t_done : &r_done),
                                           total = (label[0] == 'T' ? t_total : r_total)]()
                                              -> hash::HashTask {
                hash::HashTask task;
                task.relative_path = rel_path_str;

                // Progress counts successful hashes only; failures are reported later.
                if (hash_file(full_path, algo, task.hash_value, task.error)) {
                    auto n = ++(*counter);
                    if (verbose) {
                        std::cout << '[' << label << "] [" << n << '/' << total << "] "
                                  << rel_path_str << std::endl;
                    }
                }

                return task;
            }));
        }

        return futures;
    };

    auto target_futures = submit_with_progress(target_scan.entries, target_path, target_pool, "T");
    auto ref_futures = submit_with_progress(ref_scan.entries, ref_path, ref_pool, "R");

    // ── 6. Collect results into sorted maps ──────────────
    std::map<std::string, hash::HashTask> target_hashes;
    std::map<std::string, hash::HashTask> ref_hashes;

    for (auto& f : target_futures) {
        auto task = f.get();
        if (!task.error.empty() && !config.quiet) {
            std::cerr << "Error hashing (target): " << task.relative_path << " - " << task.error
                      << '\n';
        }
        target_hashes[task.relative_path] = std::move(task);
    }

    for (auto& f : ref_futures) {
        auto task = f.get();
        if (!task.error.empty() && !config.quiet) {
            std::cerr << "Error hashing (reference): " << task.relative_path << " - "
                      << task.error << '\n';
        }
        ref_hashes[task.relative_path] = std::move(task);
    }

    if (!config.quiet) {
        std::cout << "Done.\n\n";
    }

    // ── 7. Compare ───────────────────────────────────────
    compare::ComparisonResult result = compare::Comparator::compare(
        target_hashes, ref_hashes, config.algorithm,
        target_path.u8string(), ref_path.u8string());

    // ── 8. Report ────────────────────────────────────────
    auto reporter = create_reporter(report_format);

    if (config.output_file.empty()) {
        reporter->report(result, std::cout);
    } else {
        std::ofstream out(config.output_file);
        if (!out) {
            std::cerr << "Error: cannot write to " << config.output_file << '\n';
            return 2;
        }
        reporter->report(result, out);
        if (!config.quiet) {
            std::cout << "Report saved to " << config.output_file << '\n';
        }
    }

    // ── 9. Optionally save per-file hash logs ────────────
    if (config.save_logs) {
        save_hash_log(target_hashes, "hashchecker_target.log");
        save_hash_log(ref_hashes, "hashchecker_reference.log");
        if (!config.quiet) {
            std::cout << "Hash logs saved to hashchecker_target.log "
                         "and hashchecker_reference.log\n";
        }
    }

    return result.identical() ? 0 : 1;
}
