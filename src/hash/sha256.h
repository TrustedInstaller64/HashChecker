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

/// SHA-256 hash algorithm implementation per FIPS 180-4.
class SHA256 : public IHasher {
public:
    SHA256() { reset(); }

    void update(const void* data, std::size_t length) override;
    void finalize() override;
    std::string hexdigest() const override;
    void reset() override;

private:
    void transform(const uint8_t block[64]);

    uint32_t state_[8] = {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
                          0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};
    uint64_t count_ = 0;       // total bits processed
    uint8_t buffer_[64] = {};
    std::size_t buffer_len_ = 0;
    bool finalized_ = false;
    std::string digest_hex_;
};

inline void SHA256::reset() {
    state_[0] = 0x6A09E667;
    state_[1] = 0xBB67AE85;
    state_[2] = 0x3C6EF372;
    state_[3] = 0xA54FF53A;
    state_[4] = 0x510E527F;
    state_[5] = 0x9B05688C;
    state_[6] = 0x1F83D9AB;
    state_[7] = 0x5BE0CD19;
    count_ = 0;
    buffer_len_ = 0;
    finalized_ = false;
    digest_hex_.clear();
}

inline void SHA256::update(const void* data, std::size_t length) {
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

inline void SHA256::finalize() {
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
    // Append length in big-endian (64-bit)
    for (int i = 7; i >= 0; --i) {
        buffer_[56 + i] = static_cast<uint8_t>(bits >> ((7 - i) * 8));
    }
    transform(buffer_);

    std::ostringstream oss;
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << std::setfill('0') << std::setw(8) << state_[i];
    }
    digest_hex_ = oss.str();
}

inline std::string SHA256::hexdigest() const {
    return digest_hex_;
}

// ── SHA-256 core transform ──────────────────────────────

namespace {
    constexpr uint32_t kSha256[64] = {
        0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
        0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
        0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
        0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
        0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
        0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
        0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
        0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
        0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
        0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
        0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
        0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
        0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
        0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
        0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
        0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
    };

    inline uint32_t sha256_rotr(uint32_t x, uint32_t n) {
        return (x >> n) | (x << (32 - n));
    }
} // anonymous namespace

inline void SHA256::transform(const uint8_t block[64]) {
    uint32_t w[64];

    // Safe unaligned read into w[0..15]
    for (int i = 0; i < 16; ++i) {
        uint32_t word;
        std::memcpy(&word, block + i * 4, 4);
        // SHA-256 uses big-endian; swap bytes
        w[i] = ((word & 0xFF) << 24) | ((word & 0xFF00) << 8) |
               ((word >> 8) & 0xFF00) | ((word >> 24) & 0xFF);
    }

    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = sha256_rotr(w[i - 15], 7) ^ sha256_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = sha256_rotr(w[i - 2], 17) ^ sha256_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];
    uint32_t f = state_[5];
    uint32_t g = state_[6];
    uint32_t h = state_[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = sha256_rotr(e, 6) ^ sha256_rotr(e, 11) ^ sha256_rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + kSha256[i] + w[i];
        uint32_t S0 = sha256_rotr(a, 2) ^ sha256_rotr(a, 13) ^ sha256_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

} // namespace hash

REGISTER_HASHER(SHA256, "sha256");
