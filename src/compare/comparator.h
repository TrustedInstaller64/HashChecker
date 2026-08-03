#pragma once

#include "compare/comparison_result.h"
#include "hash/parallel_computer.h"
#include <map>
#include <string>

namespace compare {

/// Compares two sorted hash maps using an O(n+m) ordered merge.
class Comparator {
public:
    /// Produce a ComparisonResult from pre-computed hash maps.
    /// @param target_map    Map of relative_path -> HashTask for the target directory.
    /// @param reference_map Map of relative_path -> HashTask for the reference directory.
    /// @param algorithm     The algorithm used (for reporting).
    /// @param target_root   Root path of the target directory.
    /// @param reference_root Root path of the reference directory.
    static ComparisonResult compare(
        const std::map<std::string, hash::HashTask>& target_map,
        const std::map<std::string, hash::HashTask>& reference_map,
        const std::string& algorithm,
        const std::string& target_root,
        const std::string& reference_root);
};

} // namespace compare
