#pragma once

#include <string>

namespace hash {

/// Holds the result of hashing a single file.
struct HashTask {
    std::string relative_path;
    std::string hash_value;
    std::string error; // empty on success
};

} // namespace hash
