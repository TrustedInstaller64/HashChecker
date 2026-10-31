#include "hash/md5.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>

static std::string md5_hash(const std::string& input) {
    hash::MD5 md5;
    md5.update(input.data(), input.size());
    md5.finalize();
    return md5.hexdigest();
}

TEST_CASE("MD5: RFC 1321 test vectors") {
    SUBCASE("empty string") {
        CHECK(md5_hash("") == "d41d8cd98f00b204e9800998ecf8427e");
    }

    SUBCASE("a") {
        CHECK(md5_hash("a") == "0cc175b9c0f1b6a831c399e269772661");
    }

    SUBCASE("abc") {
        CHECK(md5_hash("abc") == "900150983cd24fb0d6963f7d28e17f72");
    }

    SUBCASE("message digest") {
        CHECK(md5_hash("message digest") == "f96b697d7cb7938d525a2f31aaf161d0");
    }

    SUBCASE("alphabet") {
        CHECK(md5_hash("abcdefghijklmnopqrstuvwxyz") == "c3fcd3d76192e4007dfb496cca67e13b");
    }
}

TEST_CASE("MD5: incremental update") {
    hash::MD5 md5;
    md5.update("ab", 2);
    md5.update("c", 1);
    md5.finalize();
    CHECK(md5.hexdigest() == "900150983cd24fb0d6963f7d28e17f72");
}

TEST_CASE("MD5: multi-block input (> 64 bytes)") {
    // 128 'a' characters
    std::string input(128, 'a');
    // Pre-computed: md5 of 128 'a's
    CHECK(md5_hash(input) == "e510683b3f5ffe4093d021808bc6ff70");
}

TEST_CASE("MD5: reset and reuse") {
    hash::MD5 md5;
    md5.update("test", 4);
    md5.finalize();
    std::string first = md5.hexdigest();

    md5.reset();
    md5.update("abc", 3);
    md5.finalize();
    std::string second = md5.hexdigest();

    CHECK(first != second);
    CHECK(second == "900150983cd24fb0d6963f7d28e17f72");
}
