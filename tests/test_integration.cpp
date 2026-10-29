#include "app.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static fs::path make_temp_dir() {
    auto path =
        fs::temp_directory_path() /
        ("hc_int_" + std::to_string(std::rand()));
    fs::create_directories(path);
    return path;
}

static void write_file(const fs::path& p, const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    out << content;
}

TEST_CASE("Integration: identical directories return 0") {
    auto dir_a = make_temp_dir();
    auto dir_b = make_temp_dir();

    write_file(dir_a / "f1.txt", "hello");
    write_file(dir_a / "sub/f2.txt", "world");
    write_file(dir_b / "f1.txt", "hello");
    write_file(dir_b / "sub/f2.txt", "world");

    AppConfig config;
    config.target_dir = dir_a.string();
    config.reference_dir = dir_b.string();
    config.algorithm = "sha256";
    config.quiet = true;

    int exit_code = run(config);
    CHECK(exit_code == 0);

    fs::remove_all(dir_a);
    fs::remove_all(dir_b);
}

TEST_CASE("Integration: different content returns 1") {
    auto dir_a = make_temp_dir();
    auto dir_b = make_temp_dir();

    write_file(dir_a / "f1.txt", "hello");
    write_file(dir_b / "f1.txt", "goodbye");

    AppConfig config;
    config.target_dir = dir_a.string();
    config.reference_dir = dir_b.string();
    config.algorithm = "sha256";
    config.quiet = true;

    int exit_code = run(config);
    CHECK(exit_code == 1);

    fs::remove_all(dir_a);
    fs::remove_all(dir_b);
}

TEST_CASE("Integration: missing file returns 1") {
    auto dir_a = make_temp_dir();
    auto dir_b = make_temp_dir();

    write_file(dir_a / "f1.txt", "hello");
    write_file(dir_a / "extra.txt", "bonus");
    write_file(dir_b / "f1.txt", "hello");

    AppConfig config;
    config.target_dir = dir_a.string();
    config.reference_dir = dir_b.string();
    config.algorithm = "sha256";
    config.quiet = true;

    int exit_code = run(config);
    CHECK(exit_code == 1);

    fs::remove_all(dir_a);
    fs::remove_all(dir_b);
}

TEST_CASE("Integration: invalid directory returns 2") {
    AppConfig config;
    config.target_dir = "/nonexistent_xyz";
    config.reference_dir = "/nonexistent_abc";
    config.quiet = true;

    int exit_code = run(config);
    CHECK(exit_code == 2);
}

TEST_CASE("Integration: invalid algorithm returns 2") {
    auto dir = make_temp_dir();

    AppConfig config;
    config.target_dir = dir.string();
    config.reference_dir = dir.string();
    config.algorithm = "nonexistent_algo";
    config.quiet = true;

    int exit_code = run(config);
    CHECK(exit_code == 2);

    fs::remove_all(dir);
}
