#include "compare/comparator.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <map>
#include <string>

using hash::HashTask;
using compare::Comparator;

static std::map<std::string, HashTask> make_map(
    const std::map<std::string, std::string>& data) {
    std::map<std::string, HashTask> result;
    for (const auto& [path, h] : data) {
        HashTask t;
        t.relative_path = path;
        t.hash_value = h;
        result[path] = std::move(t);
    }
    return result;
}

TEST_CASE("Comparator: identical directories") {
    auto target = make_map({{"a.txt", "abc"}, {"b.txt", "def"}});
    auto ref = make_map({{"a.txt", "abc"}, {"b.txt", "def"}});

    auto result = Comparator::compare(target, ref, "sha256", "/t", "/r");
    CHECK(result.identical());
    CHECK(result.matched == 2);
    CHECK(result.hash_mismatch == 0);
    CHECK(result.only_in_target == 0);
    CHECK(result.only_in_reference == 0);
}

TEST_CASE("Comparator: hash mismatch") {
    auto target = make_map({{"a.txt", "abc"}});
    auto ref = make_map({{"a.txt", "xyz"}});

    auto result = Comparator::compare(target, ref, "sha256", "/t", "/r");
    CHECK_FALSE(result.identical());
    CHECK(result.hash_mismatch == 1);
    CHECK(result.matched == 0);
    CHECK(result.differences.size() == 1);
    CHECK(result.differences[0].kind == compare::DiffKind::kHashMismatch);
}

TEST_CASE("Comparator: only in target") {
    auto target = make_map({{"a.txt", "abc"}, {"extra.txt", "xyz"}});
    auto ref = make_map({{"a.txt", "abc"}});

    auto result = Comparator::compare(target, ref, "sha256", "/t", "/r");
    CHECK_FALSE(result.identical());
    CHECK(result.only_in_target == 1);
    CHECK(result.matched == 1);
    CHECK(result.differences[0].kind == compare::DiffKind::kOnlyInTarget);
}

TEST_CASE("Comparator: only in reference") {
    auto target = make_map({{"a.txt", "abc"}});
    auto ref = make_map({{"a.txt", "abc"}, {"extra.txt", "xyz"}});

    auto result = Comparator::compare(target, ref, "sha256", "/t", "/r");
    CHECK_FALSE(result.identical());
    CHECK(result.only_in_reference == 1);
    CHECK(result.matched == 1);
    CHECK(result.differences[0].kind == compare::DiffKind::kOnlyInReference);
}

TEST_CASE("Comparator: empty maps") {
    std::map<std::string, HashTask> empty_t, empty_r;
    auto result = Comparator::compare(empty_t, empty_r, "sha256", "/t", "/r");
    CHECK(result.identical());
    CHECK(result.matched == 0);
}

TEST_CASE("Comparator: mixed scenario") {
    auto target = make_map({
        {"same.txt", "abc"},
        {"diff.txt", "111"},
        {"t_only.txt", "t"}
    });
    auto ref = make_map({
        {"same.txt", "abc"},
        {"diff.txt", "222"},
        {"r_only.txt", "r"}
    });

    auto result = Comparator::compare(target, ref, "sha256", "/t", "/r");
    CHECK_FALSE(result.identical());
    CHECK(result.matched == 1);
    CHECK(result.hash_mismatch == 1);
    CHECK(result.only_in_target == 1);
    CHECK(result.only_in_reference == 1);
    CHECK(result.differences.size() == 3);
}
