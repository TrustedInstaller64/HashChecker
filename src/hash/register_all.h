#pragma once

/// Include this header to ensure all built-in hash algorithms are registered.
/// This prevents the linker from stripping the static registration variables.

#include "hash/md5.h"
#include "hash/sha1.h"
#include "hash/sha256.h"
