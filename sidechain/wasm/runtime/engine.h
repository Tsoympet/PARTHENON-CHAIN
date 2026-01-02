#pragma once

#include <vector>

#include "types.h"

#include "../gas/model.h"
#include "../validator/validator.h"
#include "../../state/state_store.h"

namespace sidechain::wasm {

class ExecutionEngine {
public:
    ExecutionResult Execute(const ExecutionRequest& request, sidechain::state::StateStore& state) const;

private:
    static constexpr size_t kMaxStack = 1024;

    bool Push(std::vector<int64_t>& stack, int64_t value, ExecutionResult& result,
              GasMeter& gas_meter) const;
    bool PopTwo(std::vector<int64_t>& stack, int64_t& a, int64_t& b, ExecutionResult& result) const;
};

}  // namespace sidechain::wasm
