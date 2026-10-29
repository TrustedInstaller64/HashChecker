#include "hash/hasher_registry.h"

namespace hash {

HasherRegistry& HasherRegistry::instance() {
    static HasherRegistry registry;
    return registry;
}

void HasherRegistry::register_hasher(std::string name, Factory factory) {
    registry_[std::move(name)] = std::move(factory);
}

std::unique_ptr<IHasher> HasherRegistry::create(const std::string& name) const {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        return nullptr;
    }
    return it->second();
}

std::vector<std::string> HasherRegistry::available_algorithms() const {
    std::vector<std::string> names;
    names.reserve(registry_.size());
    for (const auto& [name, _] : registry_) {
        names.push_back(name);
    }
    return names;
}

} // namespace hash
