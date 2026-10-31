#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace scan {

/// A single file found during scanning.
struct ScanEntry {
    std::filesystem::path relative_path;
};

/// A non-fatal error encountered during scanning.
struct ScanError {
    std::filesystem::path relative_path;
    std::string message;
};

/// The result of a directory scan.
struct ScanOutcome {
    std::vector<ScanEntry> entries; // sorted by relative_path
    std::uintmax_t total_bytes = 0;
    std::vector<ScanError> errors;
};

/// Recursively scans a directory for regular files.
class FileScanner {
public:
    /// Scan all regular files in `root`.
    static ScanOutcome scan(const std::filesystem::path& root);
};

} // namespace scan
