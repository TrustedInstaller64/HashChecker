#include "compare/comparator.h"

namespace compare {

ComparisonResult Comparator::compare(
    const std::map<std::string, hash::HashTask>& target_map,
    const std::map<std::string, hash::HashTask>& reference_map,
    const std::string& algorithm,
    const std::string& target_root,
    const std::string& reference_root) {

    ComparisonResult result;
    result.algorithm = algorithm;
    result.target_root = target_root;
    result.reference_root = reference_root;

    auto t_it = target_map.begin();
    auto r_it = reference_map.begin();

    while (t_it != target_map.end() && r_it != reference_map.end()) {
        const auto& t_key = t_it->first;
        const auto& r_key = r_it->first;

        if (t_key < r_key) {
            // File only in target
            DiffEntry diff;
            diff.relative_path = t_key;
            diff.target_hash = t_it->second.hash_value;
            diff.reference_hash = "-";
            diff.kind = DiffKind::kOnlyInTarget;
            result.differences.push_back(std::move(diff));
            ++result.only_in_target;
            ++t_it;
        } else if (r_key < t_key) {
            // File only in reference
            DiffEntry diff;
            diff.relative_path = r_key;
            diff.target_hash = "-";
            diff.reference_hash = r_it->second.hash_value;
            diff.kind = DiffKind::kOnlyInReference;
            result.differences.push_back(std::move(diff));
            ++result.only_in_reference;
            ++r_it;
        } else {
            // Same path in both
            if (t_it->second.hash_value == r_it->second.hash_value) {
                ++result.matched;
            } else {
                DiffEntry diff;
                diff.relative_path = t_key;
                diff.target_hash = t_it->second.hash_value;
                diff.reference_hash = r_it->second.hash_value;
                diff.kind = DiffKind::kHashMismatch;
                result.differences.push_back(std::move(diff));
                ++result.hash_mismatch;
            }
            ++t_it;
            ++r_it;
        }
    }

    // Drain remaining entries in target
    while (t_it != target_map.end()) {
        DiffEntry diff;
        diff.relative_path = t_it->first;
        diff.target_hash = t_it->second.hash_value;
        diff.reference_hash = "-";
        diff.kind = DiffKind::kOnlyInTarget;
        result.differences.push_back(std::move(diff));
        ++result.only_in_target;
        ++t_it;
    }

    // Drain remaining entries in reference
    while (r_it != reference_map.end()) {
        DiffEntry diff;
        diff.relative_path = r_it->first;
        diff.target_hash = "-";
        diff.reference_hash = r_it->second.hash_value;
        diff.kind = DiffKind::kOnlyInReference;
        result.differences.push_back(std::move(diff));
        ++result.only_in_reference;
        ++r_it;
    }

    return result;
}

} // namespace compare
