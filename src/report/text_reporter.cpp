#include "report/text_reporter.h"

#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace report {

namespace {
    // ANSI colour codes
    const char* kReset = "\033[0m";
    const char* kRed = "\033[31m";
    const char* kGreen = "\033[32m";
    const char* kYellow = "\033[33m";
    const char* kCyan = "\033[36m";
    const char* kBold = "\033[1m";
} // namespace

bool TextReporter::is_terminal(const std::ostream& out) {
    if (out.rdbuf() == std::cout.rdbuf()) {
        return ISATTY(FILENO(stdout)) != 0;
    }
    if (out.rdbuf() == std::cerr.rdbuf()) {
        return ISATTY(FILENO(stderr)) != 0;
    }
    return false;
}

void TextReporter::report(const compare::ComparisonResult& result, std::ostream& out) {
    bool use_colour = is_terminal(out);

    const char* green = use_colour ? kGreen : "";
    const char* red = use_colour ? kRed : "";
    const char* yellow = use_colour ? kYellow : "";
    const char* cyan = use_colour ? kCyan : "";
    const char* bold = use_colour ? kBold : "";
    const char* reset = use_colour ? kReset : "";

    // Header
    out << bold << "=== HashChecker Report ===\n"
        << reset << "Algorithm:    " << result.algorithm << '\n'
        << "Target:       " << result.target_root << '\n'
        << "Reference:    " << result.reference_root << '\n'
        << '\n';

    // Summary
    std::size_t total = result.matched + result.only_in_target +
                        result.only_in_reference + result.hash_mismatch;
    out << "Total files:     " << total << '\n';
    out << green << "Matched:         " << result.matched << reset << '\n';
    if (result.only_in_target > 0) {
        out << red << "Only in target:  " << result.only_in_target << reset << '\n';
    }
    if (result.only_in_reference > 0) {
        out << red << "Only in ref:     " << result.only_in_reference << reset << '\n';
    }
    if (result.hash_mismatch > 0) {
        out << yellow << "Hash mismatch:   " << result.hash_mismatch << reset << '\n';
    }

    if (result.identical()) {
        out << '\n' << green << bold << "RESULT: IDENTICAL"
            << reset << '\n';
        return;
    }

    out << '\n' << red << bold << "RESULT: DIFFERENCES FOUND"
        << reset << "\n\n";

    // Differences table
    out << cyan << std::left
        << std::setw(12) << "STATUS"
        << std::setw(50) << "PATH"
        << std::setw(34) << "TARGET HASH"
        << "REFERENCE HASH" << reset << '\n';
    out << std::string(12 + 50 + 34 + 34, '-') << '\n';

    for (const auto& diff : result.differences) {
        std::string status;
        const char* colour = "";
        switch (diff.kind) {
        case compare::DiffKind::kOnlyInTarget:
            status = "TARGET_ONLY";
            colour = red;
            break;
        case compare::DiffKind::kOnlyInReference:
            status = "REF_ONLY";
            colour = red;
            break;
        case compare::DiffKind::kHashMismatch:
            status = "MISMATCH";
            colour = yellow;
            break;
        }

        std::string path = diff.relative_path;
        if (path.length() > 49) {
            path = "..." + path.substr(path.length() - 46);
        }

        out << colour << std::left
            << std::setw(12) << status
            << std::setw(50) << path
            << std::setw(34) << diff.target_hash
            << diff.reference_hash << reset << '\n';
    }

    out << '\n';
}

} // namespace report
