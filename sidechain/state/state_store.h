#pragma once

#include <array>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../wasm/runtime/types.h"

namespace sidechain::state {

using sidechain::wasm::ExecutionDomain;

class StateStore {
public:
    bool Put(ExecutionDomain domain, const std::string& module, const std::string& key,
             const std::vector<uint8_t>& value);
    std::vector<uint8_t> Get(ExecutionDomain domain, const std::string& module,
                             const std::string& key) const;
    bool Exists(ExecutionDomain domain, const std::string& module, const std::string& key) const;
    bool AppendEvent(ExecutionDomain domain, const std::string& module,
                     const std::string& payload);
    std::array<uint8_t, 32> ModuleRoot(ExecutionDomain domain,
                                       const std::string& module) const;
    std::array<uint8_t, 32> DomainRoot(ExecutionDomain domain) const;

private:
    struct ModuleState {
        std::unordered_map<std::string, std::vector<uint8_t>> kv;
    };

    std::map<ExecutionDomain, std::unordered_map<std::string, ModuleState>> data_;
};

}  // namespace sidechain::state
