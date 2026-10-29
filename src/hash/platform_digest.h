#pragma once

/// Platform-accelerated digest backends.
///
/// Included by register_all.h when HASHCHECKER_PLATFORM_DIGEST selects a native
/// crypto library instead of the portable implementations in md5.h/sha1.h/sha256.h.
/// On Apple platforms CommonCrypto's SHA-1/SHA-256 use the ARMv8 crypto extensions
/// (~10x the scalar code) and MD5 is still ~1.4x faster.
///
/// The portable classes stay in use by the unit tests, so both implementations
/// are exercised: the tests hash the RFC vectors through md5.h/sha1.h/sha256.h,
/// while the CLI (and the end-to-end script) hash through these wrappers.

#include "hash/hasher_registry.h"
#include "hash/ihasher.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#if defined(HASHCHECKER_USE_COMMONCRYPTO)
#include <CommonCrypto/CommonDigest.h>
#elif defined(HASHCHECKER_USE_BCRYPT)
#include <windows.h>
#include <bcrypt.h>
#else
#error "platform_digest.h requires HASHCHECKER_USE_COMMONCRYPTO or HASHCHECKER_USE_BCRYPT"
#endif

namespace hash {
namespace detail {

/// Lowercase hex encoding, two characters per byte via a lookup table.
inline std::string to_hex(const std::uint8_t* bytes, std::size_t length) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out(length * 2, '\0');
    for (std::size_t i = 0; i < length; ++i) {
        out[2 * i] = kHex[bytes[i] >> 4];
        out[2 * i + 1] = kHex[bytes[i] & 0x0F];
    }
    return out;
}

} // namespace detail

#if defined(HASHCHECKER_USE_COMMONCRYPTO)

// CommonCrypto's digest functions are deprecated in favour of CryptoKit, but
// the C API is still shipped and remains the only portable-to-Apple option.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

class MD5CommonCrypto final : public IHasher {
public:
    MD5CommonCrypto() { reset(); }

    void update(const void* data, std::size_t length) override {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        while (length > 0) {
            const auto chunk = static_cast<CC_LONG>(std::min<std::size_t>(length, 0xFFFFFFFFu));
            CC_MD5_Update(&ctx_, bytes, chunk);
            bytes += chunk;
            length -= chunk;
        }
    }

    void finalize() override {
        if (finalized_) return;
        finalized_ = true;
        std::uint8_t digest[CC_MD5_DIGEST_LENGTH];
        CC_MD5_CTX copy = ctx_;
        CC_MD5_Final(digest, &copy);
        digest_hex_ = detail::to_hex(digest, sizeof digest);
    }

    std::string hexdigest() const override { return digest_hex_; }

    void reset() override {
        CC_MD5_Init(&ctx_);
        finalized_ = false;
        digest_hex_.clear();
    }

private:
    CC_MD5_CTX ctx_{};
    bool finalized_ = false;
    std::string digest_hex_;
};

class SHA1CommonCrypto final : public IHasher {
public:
    SHA1CommonCrypto() { reset(); }

    void update(const void* data, std::size_t length) override {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        while (length > 0) {
            const auto chunk = static_cast<CC_LONG>(std::min<std::size_t>(length, 0xFFFFFFFFu));
            CC_SHA1_Update(&ctx_, bytes, chunk);
            bytes += chunk;
            length -= chunk;
        }
    }

    void finalize() override {
        if (finalized_) return;
        finalized_ = true;
        std::uint8_t digest[CC_SHA1_DIGEST_LENGTH];
        CC_SHA1_CTX copy = ctx_;
        CC_SHA1_Final(digest, &copy);
        digest_hex_ = detail::to_hex(digest, sizeof digest);
    }

    std::string hexdigest() const override { return digest_hex_; }

    void reset() override {
        CC_SHA1_Init(&ctx_);
        finalized_ = false;
        digest_hex_.clear();
    }

private:
    CC_SHA1_CTX ctx_{};
    bool finalized_ = false;
    std::string digest_hex_;
};

class SHA256CommonCrypto final : public IHasher {
public:
    SHA256CommonCrypto() { reset(); }

    void update(const void* data, std::size_t length) override {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        while (length > 0) {
            const auto chunk = static_cast<CC_LONG>(std::min<std::size_t>(length, 0xFFFFFFFFu));
            CC_SHA256_Update(&ctx_, bytes, chunk);
            bytes += chunk;
            length -= chunk;
        }
    }

    void finalize() override {
        if (finalized_) return;
        finalized_ = true;
        std::uint8_t digest[CC_SHA256_DIGEST_LENGTH];
        CC_SHA256_CTX copy = ctx_;
        CC_SHA256_Final(digest, &copy);
        digest_hex_ = detail::to_hex(digest, sizeof digest);
    }

    std::string hexdigest() const override { return digest_hex_; }

    void reset() override {
        CC_SHA256_Init(&ctx_);
        finalized_ = false;
        digest_hex_.clear();
    }

private:
    CC_SHA256_CTX ctx_{};
    bool finalized_ = false;
    std::string digest_hex_;
};

#pragma clang diagnostic pop

#endif // HASHCHECKER_USE_COMMONCRYPTO

} // namespace hash

REGISTER_HASHER(MD5CommonCrypto, "md5");
REGISTER_HASHER(SHA1CommonCrypto, "sha1");
REGISTER_HASHER(SHA256CommonCrypto, "sha256");
