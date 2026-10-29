#pragma once

#include "hash/hasher_registry.h"
#include "hash/ihasher.h"
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
    inline uint32_t rotate_left(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }
} // anonymous namespace

// The round loop this replaced was branchy (`if (i < 16) ...` per round) and used
// `% 16` indexing, which kept MD5 at roughly half of OpenSSL's throughput. Fully
// unrolling the four rounds removes both costs; the digest is unchanged.
#define MD5_F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))
#define MD5_STEP(f, a, b, c, d, x, s, k)     \
    do {                                     \
        (a) += f((b), (c), (d)) + (x) + (k); \
        (a) = rotate_left((a), (s));         \
        (a) += (b);                          \
    } while (0)

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

    // Round 1: F(b, c, d) = (b & c) | (~b & d)
    MD5_STEP(MD5_F, a, b, c, d, w[0], 7, 0xD76AA478u);
    MD5_STEP(MD5_F, d, a, b, c, w[1], 12, 0xE8C7B756u);
    MD5_STEP(MD5_F, c, d, a, b, w[2], 17, 0x242070DBu);
    MD5_STEP(MD5_F, b, c, d, a, w[3], 22, 0xC1BDCEEEu);
    MD5_STEP(MD5_F, a, b, c, d, w[4], 7, 0xF57C0FAFu);
    MD5_STEP(MD5_F, d, a, b, c, w[5], 12, 0x4787C62Au);
    MD5_STEP(MD5_F, c, d, a, b, w[6], 17, 0xA8304613u);
    MD5_STEP(MD5_F, b, c, d, a, w[7], 22, 0xFD469501u);
    MD5_STEP(MD5_F, a, b, c, d, w[8], 7, 0x698098D8u);
    MD5_STEP(MD5_F, d, a, b, c, w[9], 12, 0x8B44F7AFu);
    MD5_STEP(MD5_F, c, d, a, b, w[10], 17, 0xFFFF5BB1u);
    MD5_STEP(MD5_F, b, c, d, a, w[11], 22, 0x895CD7BEu);
    MD5_STEP(MD5_F, a, b, c, d, w[12], 7, 0x6B901122u);
    MD5_STEP(MD5_F, d, a, b, c, w[13], 12, 0xFD987193u);
    MD5_STEP(MD5_F, c, d, a, b, w[14], 17, 0xA679438Eu);
    MD5_STEP(MD5_F, b, c, d, a, w[15], 22, 0x49B40821u);

    // Round 2: G(b, c, d) = (b & d) | (c & ~d)
    MD5_STEP(MD5_G, a, b, c, d, w[1], 5, 0xF61E2562u);
    MD5_STEP(MD5_G, d, a, b, c, w[6], 9, 0xC040B340u);
    MD5_STEP(MD5_G, c, d, a, b, w[11], 14, 0x265E5A51u);
    MD5_STEP(MD5_G, b, c, d, a, w[0], 20, 0xE9B6C7AAu);
    MD5_STEP(MD5_G, a, b, c, d, w[5], 5, 0xD62F105Du);
    MD5_STEP(MD5_G, d, a, b, c, w[10], 9, 0x02441453u);
    MD5_STEP(MD5_G, c, d, a, b, w[15], 14, 0xD8A1E681u);
    MD5_STEP(MD5_G, b, c, d, a, w[4], 20, 0xE7D3FBC8u);
    MD5_STEP(MD5_G, a, b, c, d, w[9], 5, 0x21E1CDE6u);
    MD5_STEP(MD5_G, d, a, b, c, w[14], 9, 0xC33707D6u);
    MD5_STEP(MD5_G, c, d, a, b, w[3], 14, 0xF4D50D87u);
    MD5_STEP(MD5_G, b, c, d, a, w[8], 20, 0x455A14EDu);
    MD5_STEP(MD5_G, a, b, c, d, w[13], 5, 0xA9E3E905u);
    MD5_STEP(MD5_G, d, a, b, c, w[2], 9, 0xFCEFA3F8u);
    MD5_STEP(MD5_G, c, d, a, b, w[7], 14, 0x676F02D9u);
    MD5_STEP(MD5_G, b, c, d, a, w[12], 20, 0x8D2A4C8Au);

    // Round 3: H(b, c, d) = b ^ c ^ d
    MD5_STEP(MD5_H, a, b, c, d, w[5], 4, 0xFFFA3942u);
    MD5_STEP(MD5_H, d, a, b, c, w[8], 11, 0x8771F681u);
    MD5_STEP(MD5_H, c, d, a, b, w[11], 16, 0x6D9D6122u);
    MD5_STEP(MD5_H, b, c, d, a, w[14], 23, 0xFDE5380Cu);
    MD5_STEP(MD5_H, a, b, c, d, w[1], 4, 0xA4BEEA44u);
    MD5_STEP(MD5_H, d, a, b, c, w[4], 11, 0x4BDECFA9u);
    MD5_STEP(MD5_H, c, d, a, b, w[7], 16, 0xF6BB4B60u);
    MD5_STEP(MD5_H, b, c, d, a, w[10], 23, 0xBEBFBC70u);
    MD5_STEP(MD5_H, a, b, c, d, w[13], 4, 0x289B7EC6u);
    MD5_STEP(MD5_H, d, a, b, c, w[0], 11, 0xEAA127FAu);
    MD5_STEP(MD5_H, c, d, a, b, w[3], 16, 0xD4EF3085u);
    MD5_STEP(MD5_H, b, c, d, a, w[6], 23, 0x04881D05u);
    MD5_STEP(MD5_H, a, b, c, d, w[9], 4, 0xD9D4D039u);
    MD5_STEP(MD5_H, d, a, b, c, w[12], 11, 0xE6DB99E5u);
    MD5_STEP(MD5_H, c, d, a, b, w[15], 16, 0x1FA27CF8u);
    MD5_STEP(MD5_H, b, c, d, a, w[2], 23, 0xC4AC5665u);

    // Round 4: I(b, c, d) = c ^ (b | ~d)
    MD5_STEP(MD5_I, a, b, c, d, w[0], 6, 0xF4292244u);
    MD5_STEP(MD5_I, d, a, b, c, w[7], 10, 0x432AFF97u);
    MD5_STEP(MD5_I, c, d, a, b, w[14], 15, 0xAB9423A7u);
    MD5_STEP(MD5_I, b, c, d, a, w[5], 21, 0xFC93A039u);
    MD5_STEP(MD5_I, a, b, c, d, w[12], 6, 0x655B59C3u);
    MD5_STEP(MD5_I, d, a, b, c, w[3], 10, 0x8F0CCC92u);
    MD5_STEP(MD5_I, c, d, a, b, w[10], 15, 0xFFEFF47Du);
    MD5_STEP(MD5_I, b, c, d, a, w[1], 21, 0x85845DD1u);
    MD5_STEP(MD5_I, a, b, c, d, w[8], 6, 0x6FA87E4Fu);
    MD5_STEP(MD5_I, d, a, b, c, w[15], 10, 0xFE2CE6E0u);
    MD5_STEP(MD5_I, c, d, a, b, w[6], 15, 0xA3014314u);
    MD5_STEP(MD5_I, b, c, d, a, w[13], 21, 0x4E0811A1u);
    MD5_STEP(MD5_I, a, b, c, d, w[4], 6, 0xF7537E82u);
    MD5_STEP(MD5_I, d, a, b, c, w[11], 10, 0xBD3AF235u);
    MD5_STEP(MD5_I, c, d, a, b, w[2], 15, 0x2AD7D2BBu);
    MD5_STEP(MD5_I, b, c, d, a, w[9], 21, 0xEB86D391u);

#undef MD5_STEP
#undef MD5_F
#undef MD5_G
#undef MD5_H
#undef MD5_I

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
}

} // namespace hash

REGISTER_HASHER(MD5, "md5");
