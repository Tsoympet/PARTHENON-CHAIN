#include "state_store.h"

#include <algorithm>
#include <openssl/sha.h>
#include <string>

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

bool StateStore::AppendEvent(ExecutionDomain domain, const std::string& module,
                             const std::string& payload) {
    auto& module_state = data_[domain][module];
    size_t index = module_state.kv.size();
    std::string key;
    do {
        key = "event-" + std::to_string(index++);
    } while (module_state.kv.count(key) != 0);
    module_state.kv[key] = std::vector<uint8_t>(payload.begin(), payload.end());
    return true;
}

namespace {
std::array<uint8_t, 32> HashBuffer(const std::string& buffer) {
    std::array<uint8_t, 32> out{};
    if (buffer.empty()) {
        return out;
    }
    SHA256(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size(), out.data());
    return out;
}
}  // namespace

std::array<uint8_t, 32> StateStore::ModuleRoot(ExecutionDomain domain,
                                               const std::string& module) const {
    const auto domain_it = data_.find(domain);
    if (domain_it == data_.end()) {
        return {};
    }
    const auto module_it = domain_it->second.find(module);
    if (module_it == domain_it->second.end()) {
        return {};
    }
    std::vector<std::pair<std::string, std::vector<uint8_t>>> entries(module_it->second.kv.begin(),
                                                                      module_it->second.kv.end());
    std::sort(entries.begin(), entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::string buffer;
    for (const auto& entry : entries) {
        buffer.append(entry.first);
        buffer.push_back('=');
        buffer.append(reinterpret_cast<const char*>(entry.second.data()), entry.second.size());
        buffer.push_back(';');
    }
    return HashBuffer(buffer);
}

std::array<uint8_t, 32> StateStore::DomainRoot(ExecutionDomain domain) const {
    const auto domain_it = data_.find(domain);
    if (domain_it == data_.end()) {
        return {};
    }
    std::vector<std::string> module_names;
    for (const auto& [name, _] : domain_it->second) {
        module_names.push_back(name);
    }
    std::sort(module_names.begin(), module_names.end());
    std::string buffer;
    for (const auto& name : module_names) {
        const auto root = ModuleRoot(domain, name);
        buffer.append(name);
        buffer.push_back(':');
        buffer.append(reinterpret_cast<const char*>(root.data()), root.size());
        buffer.push_back('|');
    }
    return HashBuffer(buffer);
}

}  // namespace sidechain::state
