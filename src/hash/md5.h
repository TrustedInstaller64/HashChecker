#pragma once

#include "hash/hasher_registry.h"
#include "hash/ihasher.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <iomanip>

namespace hash {

/// MD5 hash algorithm implementation per RFC 1321.
class MD5 : public IHasher {
public:
    MD5() { reset(); }

    void update(const void* data, std::size_t length) override;
    void finalize() override;
    std::string hexdigest() const override;
    void reset() override;

private:
    void transform(const uint8_t block[64]);

    uint32_t state_[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    uint64_t count_ = 0;       // total bits processed
    uint8_t buffer_[64] = {};  // pending input buffer
    std::size_t buffer_len_ = 0;
    bool finalized_ = false;
    std::string digest_hex_;
};

inline void MD5::reset() {
    state_[0] = 0x67452301;
    state_[1] = 0xEFCDAB89;
    state_[2] = 0x98BADCFE;
    state_[3] = 0x10325476;
    count_ = 0;
    buffer_len_ = 0;
    finalized_ = false;
    digest_hex_.clear();
}

inline void MD5::update(const void* data, std::size_t length) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    count_ += length * 8;

    // If there's pending data in the buffer, try to fill it
    if (buffer_len_ > 0) {
        std::size_t fill = 64 - buffer_len_;
        if (length < fill) {
            std::memcpy(buffer_ + buffer_len_, bytes, length);
            buffer_len_ += length;
            return;
        }
        std::memcpy(buffer_ + buffer_len_, bytes, fill);
        transform(buffer_);
        bytes += fill;
        length -= fill;
        buffer_len_ = 0;
    }

    // Process full 64-byte blocks directly
    while (length >= 64) {
        transform(bytes);
        bytes += 64;
        length -= 64;
    }

    // Save remaining bytes
    if (length > 0) {
        std::memcpy(buffer_, bytes, length);
        buffer_len_ = length;
    }
}

inline void MD5::finalize() {
    if (finalized_) return;
    finalized_ = true;

    // Padding: append 0x80, then zeros, then length in bits (little-endian)
    uint64_t bits = count_;
    buffer_[buffer_len_++] = 0x80;

    if (buffer_len_ > 56) {
        std::memset(buffer_ + buffer_len_, 0, 64 - buffer_len_);
        transform(buffer_);
        buffer_len_ = 0;
    }

    std::memset(buffer_ + buffer_len_, 0, 56 - buffer_len_);
    std::memcpy(buffer_ + 56, &bits, 8);  // length in bits, little-endian
    transform(buffer_);

    // Build hex string
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            oss << std::setw(2) << static_cast<int>((state_[i] >> (j * 8)) & 0xFF);
        }
    }
    digest_hex_ = oss.str();
}

inline std::string MD5::hexdigest() const {
    return digest_hex_;
}

// ── MD5 core transform ──────────────────────────────────

namespace {
    constexpr uint32_t kS[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };

    constexpr uint32_t kK[64] = {
        0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
        0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
        0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
        0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
        0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
        0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
        0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
        0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
        0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
        0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
        0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
        0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
        0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
        0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
        0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
        0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
    };

    inline uint32_t rotate_left(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }
} // anonymous namespace

inline void MD5::transform(const uint8_t block[64]) {
    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];

    // Decode block into 16 uint32_t words (little-endian, safe unaligned)
    uint32_t w[16];
    for (int i = 0; i < 16; ++i) {
        std::memcpy(&w[i], block + i * 4, 4);
    }

    for (int i = 0; i < 64; ++i) {
        uint32_t f, g;
        if (i < 16) {
            f = (b & c) | ((~b) & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | ((~d) & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | (~d));
            g = (7 * i) % 16;
        }

        uint32_t temp = d;
        d = c;
        c = b;
        b = b + rotate_left(a + f + kK[i] + w[g], kS[i]);
        a = temp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
}

} // namespace hash

REGISTER_HASHER(MD5, "md5");
