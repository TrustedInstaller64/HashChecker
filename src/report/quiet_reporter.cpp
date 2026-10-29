#include "report/quiet_reporter.h"

#include <string>
#include <vector>

namespace report {

void QuietReporter::report(const compare::ComparisonResult& result, std::ostream& out) {
    if (result.identical()) {
        out << "OK: " << result.matched << " files identical.\n";
        return;
    }

    out << "DIFFER: ";
    std::vector<std::string> parts;
    if (result.hash_mismatch > 0) {
        parts.push_back(std::to_string(result.hash_mismatch) + " mismatches");
    }
    if (result.only_in_target > 0) {
        parts.push_back(std::to_string(result.only_in_target) + " only in target");
    }
    if (result.only_in_reference > 0) {
        parts.push_back(std::to_string(result.only_in_reference) + " only in reference");
    }

    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) out << ", ";
        out << parts[i];
    }
    out << ".\n";
}

} // namespace report
