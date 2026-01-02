#include "state_store.h"

namespace sidechain::state {

bool StateStore::Put(ExecutionDomain domain, const std::string& module, const std::string& key,
                     const std::vector<uint8_t>& value) {
    auto& module_state = data_[domain][module];
    module_state.kv[key] = value;
    return true;
}

std::vector<uint8_t> StateStore::Get(ExecutionDomain domain, const std::string& module,
                                     const std::string& key) const {
    const auto domain_it = data_.find(domain);
    if (domain_it == data_.end()) {
        return {};
    }
    const auto module_it = domain_it->second.find(module);
    if (module_it == domain_it->second.end()) {
        return {};
    }
    const auto kv_it = module_it->second.kv.find(key);
    if (kv_it == module_it->second.kv.end()) {
        return {};
    }
    return kv_it->second;
}

bool StateStore::Exists(ExecutionDomain domain, const std::string& module,
                        const std::string& key) const {
    const auto domain_it = data_.find(domain);
    if (domain_it == data_.end()) {
        return false;
    }
    const auto module_it = domain_it->second.find(module);
    if (module_it == domain_it->second.end()) {
        return false;
    }
    return module_it->second.kv.count(key) != 0;
}

}  // namespace sidechain::state
