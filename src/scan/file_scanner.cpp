#include "scan/file_scanner.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace scan {

#ifdef _WIN32

/// Windows: use FindFirstFileW/FindNextFileW directly.
/// MSVC's std::filesystem::directory_iterator truncates paths at CJK characters
/// on certain versions, so we bypass it entirely.
static ScanOutcome scan_impl(const fs::path& root, const ScanFilter* filter) {
    ScanOutcome outcome;
    outcome.root = root;

    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        outcome.errors.push_back(ScanError{"", "not a directory"});
        return outcome;
    }

    std::vector<fs::path> dirs;
    dirs.push_back(root);

    while (!dirs.empty()) {
        fs::path current_dir = std::move(dirs.back());
        dirs.pop_back();

        std::wstring pattern = current_dir.wstring() + L"\\*";

        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE) {
            continue;
        }

        do {
            // Skip . and ..
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
                continue;
            }

            fs::path entry_path = current_dir / fd.cFileName;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                    dirs.push_back(entry_path);
                }
            } else if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                auto rel = fs::relative(entry_path, root, ec);
                if (ec) { rel = entry_path.filename(); ec.clear(); }

                std::uintmax_t size =
                    (static_cast<std::uintmax_t>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;

                if (!filter || filter->accept(rel, size)) {
                    ScanEntry se;
                    se.relative_path = std::move(rel);
                    se.file_size = size;
                    outcome.entries.push_back(std::move(se));
                    outcome.total_bytes += size;
                }
            }
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
    }

    std::sort(outcome.entries.begin(), outcome.entries.end(),
              [](const ScanEntry& a, const ScanEntry& b) {
                  return a.relative_path < b.relative_path;
              });

    return outcome;
}

#else

/// POSIX: standard std::filesystem works fine.
static ScanOutcome scan_impl(const fs::path& root, const ScanFilter* filter) {
    ScanOutcome outcome;
    outcome.root = root;

    std::error_code ec;
    if (!fs::is_directory(root, ec)) {
        outcome.errors.push_back(ScanError{"", "not a directory"});
        return outcome;
    }

    std::vector<fs::path> dirs;
    dirs.push_back(root);

    while (!dirs.empty()) {
        fs::path current_dir = std::move(dirs.back());
        dirs.pop_back();

        auto it = fs::directory_iterator(current_dir, ec);
        if (ec) { ec.clear(); continue; }

        for (; it != fs::directory_iterator(); ++it) {
            const auto& entry = *it;

            if (entry.is_directory(ec)) {
                if (!ec) dirs.push_back(entry.path());
                ec.clear();
            } else if (entry.is_regular_file(ec)) {
                auto rel = fs::relative(entry.path(), root, ec);
                if (ec) { rel = entry.path().filename(); ec.clear(); }

                std::uintmax_t size = entry.file_size(ec);
                if (ec) { size = 0; ec.clear(); }

                if (!filter || filter->accept(rel, size)) {
                    ScanEntry se;
                    se.relative_path = std::move(rel);
                    se.file_size = size;
                    outcome.entries.push_back(std::move(se));
                    outcome.total_bytes += size;
                }
            }
        }
    }

    std::sort(outcome.entries.begin(), outcome.entries.end(),
              [](const ScanEntry& a, const ScanEntry& b) {
                  return a.relative_path < b.relative_path;
              });

    return outcome;
}

#endif

ScanOutcome FileScanner::scan(const fs::path& root) {
    return scan_impl(root, nullptr);
}

ScanOutcome FileScanner::scan(const fs::path& root, const ScanFilter& filter) {
    return scan_impl(root, &filter);
}

} // namespace scan
