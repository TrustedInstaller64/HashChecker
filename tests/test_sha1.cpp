#include "hash/sha1.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>

static std::string sha1_hash(const std::string& input) {
    hash::SHA1 sha1;
    sha1.update(input.data(), input.size());
    sha1.finalize();
    return sha1.hexdigest();
}

TEST_CASE("SHA1: FIPS 180-4 test vectors") {
    SUBCASE("empty string") {
        CHECK(sha1_hash("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    }

    SUBCASE("abc") {
        CHECK(sha1_hash("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d");
    }

    SUBCASE("448-bit input") {
        std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        CHECK(sha1_hash(input) == "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
    }
}

TEST_CASE("SHA1: incremental update") {
    hash::SHA1 sha1;
    sha1.update("ab", 2);
    sha1.update("c", 1);
    sha1.finalize();
    CHECK(sha1.hexdigest() == "a9993e364706816aba3e25717850c26c9cd0d89d");
}

TEST_CASE("SHA1: multi-block input") {
    // 128 'a' characters
    std::string input(128, 'a');
    CHECK(sha1_hash(input) == "ad5b3fdbcb526778c2839d2f151ea753995e26a0");
}

TEST_CASE("SHA1: reset and reuse") {
    hash::SHA1 sha1;
    sha1.update("test", 4);
    sha1.finalize();
    std::string first = sha1.hexdigest();

    sha1.reset();
    sha1.update("abc", 3);
    sha1.finalize();
    std::string second = sha1.hexdigest();

    CHECK(first != second);
    CHECK(second == "a9993e364706816aba3e25717850c26c9cd0d89d");
}
