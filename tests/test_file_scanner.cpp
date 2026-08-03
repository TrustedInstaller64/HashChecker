#include "scan/file_scanner.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

static fs::path make_temp_dir() {
    auto path =
        fs::temp_directory_path() /
        ("hc_test_" + std::to_string(std::rand()));
    fs::create_directories(path);
    return path;
}

static void write_file(const fs::path& p, const std::string& content) {
    std::ofstream out(p);
    out << content;
}

TEST_CASE("FileScanner: empty directory") {
    auto dir = make_temp_dir();
    auto result = scan::FileScanner::scan(dir);
    CHECK(result.entries.empty());
    CHECK(result.total_bytes == 0);
    fs::remove_all(dir);
}

TEST_CASE("FileScanner: flat directory with files") {
    auto dir = make_temp_dir();
    write_file(dir / "a.txt", "hello");
    write_file(dir / "b.txt", "world!");

    auto result = scan::FileScanner::scan(dir);
    CHECK(result.entries.size() == 2);
    CHECK(result.total_bytes == 11); // 5 + 6

    // Verify sorting
    CHECK(result.entries[0].relative_path == "a.txt");
    CHECK(result.entries[1].relative_path == "b.txt");

    fs::remove_all(dir);
}

TEST_CASE("FileScanner: nested directories") {
    auto dir = make_temp_dir();
    auto sub = dir / "sub";
    fs::create_directory(sub);
    write_file(dir / "root.txt", "root");
    write_file(sub / "nested.txt", "nested");

    auto result = scan::FileScanner::scan(dir);
    CHECK(result.entries.size() == 2);

    // Both files should be found (fs::path handles / vs \ on each platform)
    std::vector<fs::path> paths;
    for (const auto& e : result.entries) {
        paths.push_back(e.relative_path);
    }
    CHECK(std::find(paths.begin(), paths.end(), "root.txt") != paths.end());
    CHECK(std::find(paths.begin(), paths.end(), fs::path("sub") / "nested.txt") != paths.end());

    fs::remove_all(dir);
}

TEST_CASE("FileScanner: non-existent directory") {
    auto result = scan::FileScanner::scan("/nonexistent_path_xyz");
    CHECK(!result.errors.empty());
}
