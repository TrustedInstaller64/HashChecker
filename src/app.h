#pragma once

#include <cstddef>
#include <string>

/// Configuration for a comparison run, typically parsed from CLI args.
struct AppConfig {
    std::string target_dir;
    std::string reference_dir;
    std::string algorithm = "md5";
    std::size_t num_threads = 0;          // 0 = auto (80% of cores)
    std::size_t target_threads = 0;       // 0 = use num_threads
    std::size_t reference_threads = 0;    // 0 = use num_threads
    std::string output_file;              // empty = stdout
    std::string report_format = "text"; // "text", "quiet", "json"
    bool verbose = false;
    bool quiet = false;
    bool save_logs = false;
};

/// Run the full scan → hash → compare → report pipeline.
/// @return 0 if identical, 1 if differences found, 2 on error.
int run(const AppConfig& config);
