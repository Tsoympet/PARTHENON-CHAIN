#include "engine.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace sidechain::wasm {

namespace {
std::vector<uint8_t> EncodeI32(int64_t value) {
    std::vector<uint8_t> out(4);
    const int64_t clamped =
        std::max<int64_t>(std::numeric_limits<int32_t>::min(),
                          std::min<int64_t>(std::numeric_limits<int32_t>::max(), value));
    const int32_t narrowed = static_cast<int32_t>(clamped);
    std::memcpy(out.data(), &narrowed, sizeof(narrowed));
    return out;
}

int64_t DecodeI32(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < 4) {
        return 0;
    }
    int32_t v = 0;
    std::memcpy(&v, bytes.data(), sizeof(v));
    return static_cast<int64_t>(v);
}
}  // namespace

bool ExecutionEngine::Push(std::vector<int64_t>& stack, int64_t value, ExecutionResult& result,
                           GasMeter& gas_meter) const {
    if (stack.size() >= kMaxStack) {
        result.error = "stack limit exceeded";
        return false;
    }
    if (!gas_meter.ConsumeMemory(sizeof(value))) {
        result.error = gas_meter.last_error();
        return false;
    }
    stack.push_back(value);
    return true;
}

bool ExecutionEngine::PopTwo(std::vector<int64_t>& stack, int64_t& a, int64_t& b,
                             ExecutionResult& result) const {
    if (stack.size() < 2) {
        result.error = "stack underflow";
        return false;
    }
    b = stack.back();
    stack.pop_back();
    a = stack.back();
    stack.pop_back();
    return true;
}

ExecutionResult ExecutionEngine::Execute(const ExecutionRequest& request,
                                         sidechain::state::StateStore& state) const {
    ExecutionResult result;
    std::string validation_error;
    if (!ValidateAssetDomain({request.domain, request.asset_id}, validation_error)) {
        result.error = validation_error;
        return result;
    }

    GasMeter gas_meter(request.gas_limit, DefaultGasSchedule());
    std::vector<int64_t> stack;
    bool halted = false;
    for (const auto& instr : request.code) {
        if (!gas_meter.Consume(instr.op)) {
            result.error = gas_meter.last_error();
            halted = true;
        } else {
            switch (instr.op) {
                case OpCode::Nop:
                    break;
                case OpCode::ConstI32:
                    if (!Push(stack, instr.immediate, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                case OpCode::AddI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    int64_t sum = 0;
#if defined(__GNUC__)
                    if (__builtin_add_overflow(a, b, &sum)) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
#else
                    const bool overflow = (b > 0 && a > std::numeric_limits<int64_t>::max() - b) ||
                                          (b < 0 && a < std::numeric_limits<int64_t>::min() - b);
                    if (overflow) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
                    sum = a + b;
#endif
                    if (!Push(stack, sum, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::SubI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    int64_t diff = 0;
#if defined(__GNUC__)
                    if (__builtin_sub_overflow(a, b, &diff)) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
#else
                    const bool overflow = (b < 0 && a > std::numeric_limits<int64_t>::max() + b) ||
                                          (b > 0 && a < std::numeric_limits<int64_t>::min() + b);
                    if (overflow) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
                    diff = a - b;
#endif
                    if (!Push(stack, diff, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::MulI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    int64_t prod = 0;
#if defined(__GNUC__)
                    if (__builtin_mul_overflow(a, b, &prod)) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
#else
                    // Check for overflow: special cases first
                    if (a == std::numeric_limits<int64_t>::min() && b == -1) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
                    if (b == std::numeric_limits<int64_t>::min() && a == -1) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
                    // For general case: |a| * |b| must not overflow
                    if (b != 0) {
                        const uint64_t absA = safe_abs(a);
                        const uint64_t absB = safe_abs(b);
                        if (absA > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) / absB) {
                            result.error = "arithmetic overflow";
                            halted = true;
                            break;
                        }
                    }
                    prod = a * b;
#endif
                    if (!Push(stack, prod, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::DivI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    if (b == 0) {
                        result.error = "division by zero";
                        halted = true;
                        break;
                    }
                    if (a == std::numeric_limits<int64_t>::min() && b == -1) {
                        result.error = "arithmetic overflow";
                        halted = true;
                        break;
                    }
                    if (!Push(stack, a / b, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::EqI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    if (!Push(stack, (a == b) ? 1 : 0, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::LtI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    if (!Push(stack, (a < b) ? 1 : 0, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::GtI32: {
                    int64_t a = 0, b = 0;
                    if (!PopTwo(stack, a, b, result)) {
                        halted = true;
                        break;
                    }
                    if (!Push(stack, (a > b) ? 1 : 0, result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::Load: {
                    const auto stored = state.Get(request.domain, request.module_id,
                                                  std::to_string(instr.immediate));
                    if (!Push(stack, DecodeI32(stored), result, gas_meter)) {
                        halted = true;
                    }
                    break;
                }
                case OpCode::Store: {
                    if (stack.empty()) {
                        result.error = "stack underflow";
                        halted = true;
                        break;
                    }
                    const int64_t value = stack.back();
                    stack.pop_back();
                    const auto encoded = EncodeI32(value);
                    if (!gas_meter.ConsumeMemory(encoded.size())) {
                        result.error = gas_meter.last_error();
                        halted = true;
                        break;
                    }
                    state.Put(request.domain, request.module_id, std::to_string(instr.immediate),
                              encoded);
                    ++result.state_writes;
                    break;
                }
                case OpCode::ReturnTop: {
                    if (stack.empty()) {
                        result.error = "stack underflow";
                    } else {
                        result.output = EncodeI32(stack.back());
                        result.success = true;
                    }
                    halted = true;
                    break;
                }
                default:
                    result.error = "unknown opcode";
                    halted = true;
                    break;
            }
        }

        if (halted) {
            break;
        }
    }

    if (result.error.empty()) {
        const bool completed = (!halted || result.success);
        result.success = result.success || completed;
    } else {
        result.success = false;
    }

    result.gas_used = gas_meter.used();
    return result;
}

}  // namespace sidechain::wasm
