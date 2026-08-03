#pragma once

#include <cstdint>
#include <filesystem>

namespace scan {

/// Abstract filter for deciding which files to include in a scan.
/// Extend this to implement extension filters, size filters, etc.
class ScanFilter {
public:
    virtual ~ScanFilter() = default;

    /// Return true if the file at `relative_path` with `file_size` bytes
    /// should be included in the scan.
    virtual bool accept(const std::filesystem::path& relative_path,
                        std::uintmax_t file_size) const = 0;
};

} // namespace scan
