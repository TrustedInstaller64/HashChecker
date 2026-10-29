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

/// SHA-1 hash algorithm implementation per FIPS 180-4.
class SHA1 : public IHasher {
public:
    SHA1() { reset(); }

    void update(const void* data, std::size_t length) override;
    void finalize() override;
    std::string hexdigest() const override;
    void reset() override;

private:
    void transform(const uint8_t block[64]);

    uint32_t state_[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    uint64_t count_ = 0;       // total bits processed
    uint8_t buffer_[64] = {};
    std::size_t buffer_len_ = 0;
    bool finalized_ = false;
    std::string digest_hex_;
};

inline void SHA1::reset() {
    state_[0] = 0x67452301;
    state_[1] = 0xEFCDAB89;
    state_[2] = 0x98BADCFE;
    state_[3] = 0x10325476;
    state_[4] = 0xC3D2E1F0;
    count_ = 0;
    buffer_len_ = 0;
    finalized_ = false;
    digest_hex_.clear();
}

inline void SHA1::update(const void* data, std::size_t length) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    count_ += length * 8;

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

    while (length >= 64) {
        transform(bytes);
        bytes += 64;
        length -= 64;
    }

    if (length > 0) {
        std::memcpy(buffer_, bytes, length);
        buffer_len_ = length;
    }
}

inline void SHA1::finalize() {
    if (finalized_) return;
    finalized_ = true;

    uint64_t bits = count_;
    buffer_[buffer_len_++] = 0x80;

    if (buffer_len_ > 56) {
        std::memset(buffer_ + buffer_len_, 0, 64 - buffer_len_);
        transform(buffer_);
        buffer_len_ = 0;
    }

    std::memset(buffer_ + buffer_len_, 0, 56 - buffer_len_);
    // Append length in big-endian (SHA-1 uses big-endian for the length)
    for (int i = 7; i >= 0; --i) {
        buffer_[56 + i] = static_cast<uint8_t>(bits >> ((7 - i) * 8));
    }
    transform(buffer_);

    std::ostringstream oss;
    for (int i = 0; i < 5; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(8) << state_[i];
    }
    digest_hex_ = oss.str();
}

inline std::string SHA1::hexdigest() const {
    return digest_hex_;
}

// ── SHA-1 core transform ────────────────────────────────

namespace {
    inline uint32_t sha1_rotl(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }
} // anonymous namespace

inline void SHA1::transform(const uint8_t block[64]) {
    uint32_t w[80];

    // Safe unaligned read into w[0..15]
    for (int i = 0; i < 16; ++i) {
        uint32_t word;
        std::memcpy(&word, block + i * 4, 4);
        // SHA-1 uses big-endian; swap bytes
        w[i] = ((word & 0xFF) << 24) | ((word & 0xFF00) << 8) |
               ((word >> 8) & 0xFF00) | ((word >> 24) & 0xFF);
    }

    for (int i = 16; i < 80; ++i) {
        w[i] = sha1_rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];

    for (int i = 0; i < 80; ++i) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = sha1_rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = sha1_rotl(b, 30);
        b = a;
        a = temp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
}

} // namespace hash

REGISTER_HASHER(SHA1, "sha1");
