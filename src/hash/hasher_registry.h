#pragma once

#include "hash/ihasher.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hash {

/// Singleton registry for hash algorithm factories.
/// New algorithms self-register via the REGISTER_HASHER macro.
class HasherRegistry {
public:
    using Factory = std::function<std::unique_ptr<IHasher>()>;

    /// Get the global registry instance (Meyer's singleton, thread-safe in C++11+).
    static HasherRegistry& instance();

    /// Register a hasher factory under the given name.
    void register_hasher(std::string name, Factory factory);

    /// Create a new hasher instance by algorithm name.
    /// Returns nullptr if the name is not registered.
    std::unique_ptr<IHasher> create(const std::string& name) const;

    /// List all registered algorithm names.
    std::vector<std::string> available_algorithms() const;

private:
    HasherRegistry() = default;
    std::map<std::string, Factory> registry_;
};

} // namespace hash

/// Macro to self-register a hasher class at static initialization time.
/// Place this at the bottom of each hash implementation header file.
/// Usage: REGISTER_HASHER(MD5, "md5")
#define REGISTER_HASHER(ClassName, Name)                                                         \
    static bool _reg_##ClassName = []() {                                                        \
        ::hash::HasherRegistry::instance().register_hasher(                                       \
            Name, [] { return std::make_unique<::hash::ClassName>(); });                          \
        return true;                                                                             \
    }()
