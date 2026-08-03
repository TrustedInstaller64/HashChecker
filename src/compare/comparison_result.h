#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace compare {

/// Describes the kind of difference between two file entries.
enum class DiffKind {
    kHashMismatch,   ///< Same path, different hash values.
    kOnlyInTarget,   ///< File only exists in the target directory.
    kOnlyInReference ///< File only exists in the reference directory.
};

/// A single difference entry between the two directory trees.
struct DiffEntry {
    std::string relative_path;
    std::string target_hash;
    std::string reference_hash;
    DiffKind kind = DiffKind::kHashMismatch;
};

/// The complete result of comparing two directory hash maps.
struct ComparisonResult {
    std::string target_root;
    std::string reference_root;
    std::string algorithm;
    std::size_t matched = 0;
    std::size_t only_in_target = 0;
    std::size_t only_in_reference = 0;
    std::size_t hash_mismatch = 0;
    std::vector<DiffEntry> differences;

    /// True when both directories have identical content.
    bool identical() const { return differences.empty(); }
};

} // namespace compare
