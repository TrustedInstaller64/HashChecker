#include "hash/sha256.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>

static std::string sha256_hash(const std::string& input) {
    hash::SHA256 sha256;
    sha256.update(input.data(), input.size());
    sha256.finalize();
    return sha256.hexdigest();
}

TEST_CASE("SHA256: FIPS 180-4 test vectors") {
    SUBCASE("empty string") {
        CHECK(sha256_hash("") ==
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }

    SUBCASE("abc") {
        CHECK(sha256_hash("abc") ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }

    SUBCASE("448-bit input") {
        std::string input = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        CHECK(sha256_hash(input) ==
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    }
}

TEST_CASE("SHA256: incremental update") {
    hash::SHA256 sha256;
    sha256.update("ab", 2);
    sha256.update("c", 1);
    sha256.finalize();
    CHECK(sha256.hexdigest() ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE("SHA256: multi-block input") {
    // 128 'a' characters
    std::string input(128, 'a');
    CHECK(sha256_hash(input) ==
          "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e");
}

TEST_CASE("SHA256: reset and reuse") {
    hash::SHA256 sha256;
    sha256.update("test", 4);
    sha256.finalize();
    std::string first = sha256.hexdigest();

    sha256.reset();
    sha256.update("abc", 3);
    sha256.finalize();
    std::string second = sha256.hexdigest();

    CHECK(first != second);
    CHECK(second ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
