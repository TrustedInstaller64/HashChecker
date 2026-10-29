#pragma once

/// Include this header to ensure all built-in hash algorithms are registered.
/// This prevents the linker from stripping the static registration variables.
///
/// When HASHCHECKER_PLATFORM_DIGEST is enabled the CLI uses the platform crypto
/// library through platform_digest.h; the portable implementations are still
/// what the unit tests compile against.

#if defined(HASHCHECKER_USE_COMMONCRYPTO) || defined(HASHCHECKER_USE_BCRYPT)
#include "hash/platform_digest.h"
#else
#include "hash/md5.h"
#include "hash/sha1.h"
#include "hash/sha256.h"
#endif
