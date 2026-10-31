#include "report/json_reporter.h"

namespace report {

std::string JsonReporter::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

void JsonReporter::report(const compare::ComparisonResult& result, std::ostream& out) {
    out << "{\n";
    out << "  \"target_root\": \"" << escape(result.target_root) << "\",\n";
    out << "  \"reference_root\": \"" << escape(result.reference_root) << "\",\n";
    out << "  \"algorithm\": \"" << escape(result.algorithm) << "\",\n";
    out << "  \"matched\": " << result.matched << ",\n";
    out << "  \"only_in_target\": " << result.only_in_target << ",\n";
    out << "  \"only_in_reference\": " << result.only_in_reference << ",\n";
    out << "  \"hash_mismatch\": " << result.hash_mismatch << ",\n";
    out << "  \"identical\": " << (result.identical() ? "true" : "false") << ",\n";
    out << "  \"differences\": [\n";

    for (std::size_t i = 0; i < result.differences.size(); ++i) {
        const auto& d = result.differences[i];
        const char* kind_str = "hash_mismatch";
        switch (d.kind) {
        case compare::DiffKind::kOnlyInTarget: kind_str = "only_in_target"; break;
        case compare::DiffKind::kOnlyInReference: kind_str = "only_in_reference"; break;
        case compare::DiffKind::kHashMismatch: kind_str = "hash_mismatch"; break;
        }

        out << "    {\n";
        out << "      \"path\": \"" << escape(d.relative_path) << "\",\n";
        out << "      \"target_hash\": \"" << escape(d.target_hash) << "\",\n";
        out << "      \"reference_hash\": \"" << escape(d.reference_hash) << "\",\n";
        out << "      \"kind\": \"" << kind_str << "\"\n";
        out << "    }";
        if (i + 1 < result.differences.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n";
    out << "}\n";
}

} // namespace report
