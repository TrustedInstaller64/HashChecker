#pragma once

/// Platform-accelerated digest backends.
///
/// Included by register_all.h when HASHCHECKER_PLATFORM_DIGEST selects a native
/// crypto library instead of the portable implementations in md5.h/sha1.h/sha256.h.
/// On Apple platforms CommonCrypto's SHA-1/SHA-256 use the ARMv8 crypto extensions
/// (~7x the scalar code) and MD5 is still ~1.3x faster. On Windows the BCrypt
/// backend is available for the same reason (hardware SHA-2 support), but it is
/// opt-in because it cannot be built or tested on non-Windows hosts.
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
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>

#include <stdexcept>
#include <vector>
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

#elif defined(HASHCHECKER_USE_BCRYPT)

/// Shared BCrypt plumbing: opens the algorithm provider once and creates a
/// fresh hash handle on reset()/finalize(). The derived classes only pick the
/// algorithm and digest length.
class BCryptHasher : public IHasher {
public:
    BCryptHasher(LPCWSTR algorithm_id, std::size_t digest_length)
        : digest_length_(digest_length) {
        check(BCryptOpenAlgorithmProvider(&alg_, algorithm_id, nullptr, 0), "open algorithm");
        ULONG object_length = 0;
        ULONG written = 0;
        check(BCryptGetProperty(alg_, BCRYPT_OBJECT_LENGTH,
                                reinterpret_cast<PUCHAR>(&object_length),
                                sizeof object_length, &written, 0),
              "query object length");
        object_.resize(object_length);
        create_hash();
    }

    ~BCryptHasher() override {
        if (hash_ != nullptr) {
            BCryptDestroyHash(hash_);
        }
        if (alg_ != nullptr) {
            BCryptCloseAlgorithmProvider(alg_, 0);
        }
    }

    void update(const void* data, std::size_t length) override {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        while (length > 0) {
            const ULONG chunk = static_cast<ULONG>(std::min<std::size_t>(length, 0xFFFFFFFFu));
            check(BCryptHashData(hash_, const_cast<PUCHAR>(bytes), chunk, 0), "hash data");
            bytes += chunk;
            length -= chunk;
        }
    }

    void finalize() override {
        if (finalized_) return;
        finalized_ = true;
        digest_.resize(digest_length_);
        check(BCryptFinishHash(hash_, digest_.data(), static_cast<ULONG>(digest_.size()), 0),
              "finish hash");
        BCryptDestroyHash(hash_);
        hash_ = nullptr;
        digest_hex_ = detail::to_hex(digest_.data(), digest_.size());
    }

    std::string hexdigest() const override { return digest_hex_; }

    void reset() override {
        if (hash_ != nullptr) {
            BCryptDestroyHash(hash_);
            hash_ = nullptr;
        }
        create_hash();
        finalized_ = false;
        digest_hex_.clear();
    }

private:
    void create_hash() {
        check(BCryptCreateHash(alg_, &hash_, object_.data(),
                               static_cast<ULONG>(object_.size()), nullptr, 0, 0),
              "create hash");
    }

    static void check(NTSTATUS status, const char* what) {
        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error(std::string("bcrypt: ") + what + " failed");
        }
    }

    BCRYPT_ALG_HANDLE alg_ = nullptr;
    BCRYPT_HASH_HANDLE hash_ = nullptr;
    std::vector<std::uint8_t> object_;
    std::vector<std::uint8_t> digest_;
    std::size_t digest_length_;
    bool finalized_ = false;
    std::string digest_hex_;
};

class MD5BCrypt final : public BCryptHasher {
public:
    MD5BCrypt() : BCryptHasher(BCRYPT_MD5_ALGORITHM, 16) {}
};

class SHA1BCrypt final : public BCryptHasher {
public:
    SHA1BCrypt() : BCryptHasher(BCRYPT_SHA1_ALGORITHM, 20) {}
};

class SHA256BCrypt final : public BCryptHasher {
public:
    SHA256BCrypt() : BCryptHasher(BCRYPT_SHA256_ALGORITHM, 32) {}
};

#endif

} // namespace hash

#if defined(HASHCHECKER_USE_COMMONCRYPTO)
REGISTER_HASHER(MD5CommonCrypto, "md5");
REGISTER_HASHER(SHA1CommonCrypto, "sha1");
REGISTER_HASHER(SHA256CommonCrypto, "sha256");
#elif defined(HASHCHECKER_USE_BCRYPT)
REGISTER_HASHER(MD5BCrypt, "md5");
REGISTER_HASHER(SHA1BCrypt, "sha1");
REGISTER_HASHER(SHA256BCrypt, "sha256");
#endif
