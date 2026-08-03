#pragma once

#include <cstddef>
#include <string>

namespace hash {

/// Abstract interface for cryptographic hash algorithms.
/// Implementations must be callable in the following order:
///   update()  (zero or more times)
///   finalize() (exactly once)
///   hexdigest() (any time after finalize)
///   reset() to start a new hash
class IHasher {
public:
    virtual ~IHasher() = default;

    /// Feed data into the hash computation. May be called multiple times.
    virtual void update(const void* data, std::size_t length) = 0;

    /// Finalize the hash. After this call, hexdigest() returns the result.
    virtual void finalize() = 0;

    /// Return the digest as a lowercase hex string. Must be called after finalize().
    virtual std::string hexdigest() const = 0;

    /// Reset the hasher state so it can be reused for a new hash.
    virtual void reset() = 0;
};

} // namespace hash
