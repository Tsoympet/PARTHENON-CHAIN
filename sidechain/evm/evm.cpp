#include "evm.h"

#include "layer1-core/crypto/schnorr.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>

using boost::multiprecision::cpp_int;

namespace {
constexpr uint64_t k_stack_limit = 1024;
constexpr unsigned k_word_bits = 256;

cpp_int mask_word(const cpp_int& value) {
    return value & ((cpp_int(1) << k_word_bits) - 1);
}

std::array<uint8_t, 32> to_bytes32(const cpp_int& value) {
    std::array<uint8_t, 32> out{};
    cpp_int masked = mask_word(value);
    for (int i = 31; i >= 0; --i) {
        out[static_cast<size_t>(i)] = static_cast<uint8_t>((masked & 0xff).convert_to<uint64_t>());
        masked >>= 8;
    }
    return out;
}

cpp_int from_bytes(const uint8_t* data, size_t size) {
    cpp_int value = 0;
    for (size_t i = 0; i < size; ++i) {
        value <<= 8;
        value += data[i];
    }
    return mask_word(value);
}

std::array<uint8_t, 32> keccak_256(const uint8_t* data, size_t size) {
    std::array<uint8_t, 32> digest{};
    using evp_md_ctx_ptr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    evp_md_ctx_ptr ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!ctx) {
        return digest;
    }
    const EVP_MD* md = EVP_sha3_256();
    if (EVP_DigestInit_ex(ctx.get(), md, nullptr) != 1) {
        return digest;
    }
    if (size > 0 && EVP_DigestUpdate(ctx.get(), data, size) != 1) {
        return digest;
    }
    unsigned int out_len = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &out_len) != 1 || out_len != digest.size()) {
        digest.fill(0);
    }
    return digest;
}

uint64_t safe_add_gas(uint64_t gas_used, uint64_t cost) {
    if (cost > std::numeric_limits<uint64_t>::max() - gas_used) {
        return std::numeric_limits<uint64_t>::max();
    }
    return gas_used + cost;
}

bool charge_gas(uint64_t& gas_used, uint64_t gas_limit, uint64_t base_cost, uint64_t drm_fee) {
    const uint64_t total_cost = safe_add_gas(base_cost, drm_fee);
    gas_used = safe_add_gas(gas_used, total_cost);
    return gas_used <= gas_limit;
}

cpp_int signed_value(const cpp_int& value) {
    const cpp_int masked = mask_word(value);
    const cpp_int sign_bit = cpp_int(1) << (k_word_bits - 1);
    if ((masked & sign_bit) != 0) {
        return masked - (cpp_int(1) << k_word_bits);
    }
    return masked;
}

bool to_uint64_safe(const cpp_int& value, uint64_t& out) {
    if (value < 0 || value > std::numeric_limits<uint64_t>::max()) {
        return false;
    }
    out = value.convert_to<uint64_t>();
    return true;
}

struct execution_context {
    std::vector<cpp_int> stack;
    std::vector<uint8_t> memory;
    std::unordered_map<cpp_int, cpp_int, evm_word_hash> transient_storage;
    std::unordered_map<cpp_int, cpp_int, evm_word_hash> storage;
    uint64_t gas_used{0};
    bool halted{false};
    std::string error;
    std::vector<uint8_t> return_data;
};

bool stack_push(execution_context& ctx, const cpp_int& value) {
    if (ctx.stack.size() >= k_stack_limit) {
        ctx.error = "stack overflow";
        ctx.halted = true;
        return false;
    }
    ctx.stack.push_back(mask_word(value));
    return true;
}

cpp_int stack_pop(execution_context& ctx) {
    if (ctx.stack.empty()) {
        ctx.error = "stack underflow";
        ctx.halted = true;
        return 0;
    }
    cpp_int value = ctx.stack.back();
    ctx.stack.pop_back();
    return value;
}

bool ensure_memory(execution_context& ctx, size_t offset, size_t size) {
    if (offset > ctx.memory.size()) {
        ctx.memory.resize(offset, 0);
    }
    if (offset + size > ctx.memory.size()) {
        ctx.memory.resize(offset + size, 0);
    }
    return true;
}

cpp_int read_stack_index(const execution_context& ctx, size_t index_from_top) {
    const size_t idx = ctx.stack.size() - 1 - index_from_top;
    return ctx.stack.at(idx);
}

bool perform_schnorr_verify(execution_context& ctx, const evm_state& state) {
    // Stack expects: [msg_len, msg_offset, sig_offset]
    const cpp_int sig_offset_int = stack_pop(ctx);
    const cpp_int msg_offset_int = stack_pop(ctx);
    const cpp_int msg_len_int = stack_pop(ctx);
    if (ctx.halted) {
        return false;
    }

    uint64_t sig_offset = 0;
    uint64_t msg_offset = 0;
    uint64_t msg_len = 0;
    if (!to_uint64_safe(sig_offset_int, sig_offset) || !to_uint64_safe(msg_offset_int, msg_offset) ||
        !to_uint64_safe(msg_len_int, msg_len)) {
        ctx.error = "offset does not fit";
        ctx.halted = true;
        return false;
    }

    if (msg_len == 0 || sig_offset + 64 > ctx.memory.size() ||
        msg_offset + msg_len > ctx.memory.size()) {
        ctx.error = "invalid schnorr arguments";
        ctx.halted = true;
        return false;
    }

    std::array<uint8_t, 64> signature{};
    std::copy_n(ctx.memory.begin() + sig_offset, signature.size(), signature.begin());
    std::vector<uint8_t> message(ctx.memory.begin() + msg_offset, ctx.memory.begin() + msg_offset + msg_len);
    const auto digest = keccak_256(message.data(), message.size());

    const bool verified = schnorr_verify(state.validator_pubkey.data(), digest.data(), signature.data());
    return stack_push(ctx, verified ? cpp_int(1) : cpp_int(0));
}

}  // namespace

std::size_t evm_word_hash::operator()(const cpp_int& value) const noexcept {
    std::array<uint8_t, 32> bytes = to_bytes32(value);
    std::size_t hash = 0;
    for (auto b : bytes) {
        hash = (hash * 131) ^ b;
    }
    return hash;
}

evm_result execute(const evm_code& bytecode, const evm_state& state, uint64_t gas_limit) {
    execution_context ctx;
    ctx.storage = state.storage;

    static const std::unordered_map<uint8_t, uint64_t> gas_schedule = {
        {0x00, 0},   // STOP
        {0x01, 3},   // ADD
        {0x02, 5},   // MUL
        {0x03, 3},   // SUB
        {0x04, 5},   // DIV
        {0x06, 5},   // MOD
        {0x07, 8},   // SMOD
        {0x08, 8},   // ADDMOD
        {0x09, 8},   // MULMOD
        {0x0a, 10},  // EXP
        {0x0b, 3},   // SIGNEXTEND
        {0x10, 3},   // LT
        {0x11, 3},   // GT
        {0x12, 3},   // SLT
        {0x13, 3},   // SGT
        {0x14, 3},   // EQ
        {0x15, 3},   // ISZERO
        {0x16, 3},   // AND
        {0x17, 3},   // OR
        {0x18, 3},   // XOR
        {0x19, 2},   // NOT
        {0x1a, 3},   // BYTE
        {0x1b, 3},   // SHL
        {0x1c, 3},   // SHR
        {0x1d, 3},   // SAR
        {0x20, 30},  // KECCAK256
        {0x50, 2},   // POP
        {0x51, 3},   // MLOAD
        {0x52, 3},   // MSTORE
        {0x53, 3},   // MSTORE8
        {0x54, 50},  // SLOAD
        {0x55, 200}, // SSTORE
        {0x56, 8},   // JUMP
        {0x57, 10},  // JUMPI
        {0x58, 1},   // PC
        {0x5b, 1},   // JUMPDEST
    };

    static const std::unordered_set<uint8_t> disabled_opcodes = {
        0xf1, // CALL
        0xf4, // DELEGATECALL
        0xff, // SELFDESTRUCT
    };

    const auto code_size = bytecode.bytes.size();
    size_t pc = 0;

    auto fetch_gas_cost = [&](uint8_t opcode) -> uint64_t {
        const auto it = gas_schedule.find(opcode);
        if (it != gas_schedule.end()) {
            return it->second;
        }
        if (opcode >= 0x60 && opcode <= 0x7f) {
            return 3;  // PUSH
        }
        if (opcode >= 0x80 && opcode <= 0x8f) {
            return 3;  // DUP
        }
        if (opcode >= 0x90 && opcode <= 0x9f) {
            return 3;  // SWAP
        }
        if (opcode == 0xf3) {
            return 0;  // RETURN
        }
        if (opcode == 0x5c || opcode == 0x5d) {
            return 30;  // TLOAD/TSTORE
        }
        if (opcode == 0xf9) {
            return 5000;  // SCHNORR_VERIFY
        }
        return 0;
    };

    while (pc < code_size && !ctx.halted) {
        const uint8_t opcode = bytecode.bytes[pc++];
        if (disabled_opcodes.count(opcode) != 0) {
            ctx.error = "opcode disabled";
            ctx.halted = true;
            break;
        }

        const uint64_t base_cost = fetch_gas_cost(opcode);
        if (!charge_gas(ctx.gas_used, gas_limit, base_cost, state.drm_fee_per_gas)) {
            ctx.error = "out of gas";
            ctx.halted = true;
            break;
        }

        switch (opcode) {
            case 0x00:  // STOP
                ctx.halted = true;
                ctx.error.clear();
                break;
            case 0x01: {  // ADD
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a + b);
                }
                break;
            }
            case 0x02: {  // MUL
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a * b);
                }
                break;
            }
            case 0x03: {  // SUB
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a - b);
                }
                break;
            }
            case 0x04: {  // DIV
                const cpp_int denominator = stack_pop(ctx);
                const cpp_int numerator = stack_pop(ctx);
                if (!ctx.halted) {
                    if (denominator == 0) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, numerator / denominator);
                    }
                }
                break;
            }
            case 0x06: {  // MOD
                const cpp_int denominator = stack_pop(ctx);
                const cpp_int numerator = stack_pop(ctx);
                if (!ctx.halted) {
                    if (denominator == 0) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, numerator % denominator);
                    }
                }
                break;
            }
            case 0x07: {  // SMOD
                const cpp_int denominator = signed_value(stack_pop(ctx));
                const cpp_int numerator = signed_value(stack_pop(ctx));
                if (!ctx.halted) {
                    if (denominator == 0) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, numerator % denominator);
                    }
                }
                break;
            }
            case 0x08: {  // ADDMOD
                const cpp_int modulus = stack_pop(ctx);
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    if (modulus == 0) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, mask_word((a + b) % modulus));
                    }
                }
                break;
            }
            case 0x09: {  // MULMOD
                const cpp_int modulus = stack_pop(ctx);
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    if (modulus == 0) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, mask_word((a * b) % modulus));
                    }
                }
                break;
            }
            case 0x0a: {  // EXP
                const cpp_int exponent = stack_pop(ctx);
                const cpp_int base = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, boost::multiprecision::pow(base, exponent));
                }
                break;
            }
            case 0x0b: {  // SIGNEXTEND
                const cpp_int byte_index = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    if (byte_index >= k_word_bits / 8) {
                        stack_push(ctx, value);
                    } else {
                        const unsigned shift = static_cast<unsigned>((k_word_bits / 8 - 1 - byte_index.convert_to<unsigned>()) * 8);
                        const cpp_int masked = mask_word(value << shift) >> shift;
                        stack_push(ctx, masked);
                    }
                }
                break;
            }
            case 0x10: {  // LT
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a < b ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x11: {  // GT
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a > b ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x12: {  // SLT
                const cpp_int a = signed_value(stack_pop(ctx));
                const cpp_int b = signed_value(stack_pop(ctx));
                if (!ctx.halted) {
                    stack_push(ctx, a < b ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x13: {  // SGT
                const cpp_int a = signed_value(stack_pop(ctx));
                const cpp_int b = signed_value(stack_pop(ctx));
                if (!ctx.halted) {
                    stack_push(ctx, a > b ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x14: {  // EQ
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, a == b ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x15: {  // ISZERO
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, value == 0 ? cpp_int(1) : cpp_int(0));
                }
                break;
            }
            case 0x16: {  // AND
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, mask_word(a & b));
                }
                break;
            }
            case 0x17: {  // OR
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, mask_word(a | b));
                }
                break;
            }
            case 0x18: {  // XOR
                const cpp_int a = stack_pop(ctx);
                const cpp_int b = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, mask_word(a ^ b));
                }
                break;
            }
            case 0x19: {  // NOT
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    stack_push(ctx, mask_word(~value));
                }
                break;
            }
            case 0x1a: {  // BYTE
                const cpp_int index = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    if (index >= 32) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        const auto bytes = to_bytes32(value);
                        stack_push(ctx, cpp_int(bytes[static_cast<size_t>(index.convert_to<unsigned>())]));
                    }
                }
                break;
            }
            case 0x1b: {  // SHL
                const cpp_int shift = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    if (shift >= k_word_bits) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, mask_word(value << shift));
                    }
                }
                break;
            }
            case 0x1c: {  // SHR
                const cpp_int shift = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    if (shift >= k_word_bits) {
                        stack_push(ctx, cpp_int(0));
                    } else {
                        stack_push(ctx, mask_word(value >> shift));
                    }
                }
                break;
            }
            case 0x1d: {  // SAR
                const cpp_int shift = stack_pop(ctx);
                const cpp_int value = signed_value(stack_pop(ctx));
                if (!ctx.halted) {
                    if (shift >= k_word_bits) {
                        stack_push(ctx, value < 0 ? cpp_int(-1) : cpp_int(0));
                    } else {
                        stack_push(ctx, mask_word(value >> shift));
                    }
                }
                break;
            }
            case 0x20: {  // KECCAK256
                const cpp_int offset_int = stack_pop(ctx);
                const cpp_int size_int = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t offset = 0;
                uint64_t size = 0;
                if (!to_uint64_safe(offset_int, offset) || !to_uint64_safe(size_int, size)) {
                    ctx.error = "keccak offset overflow";
                    ctx.halted = true;
                    break;
                }
                if (offset + size > ctx.memory.size()) {
                    ctx.error = "keccak out of bounds";
                    ctx.halted = true;
                    break;
                }
                const auto digest = keccak_256(ctx.memory.data() + offset, size);
                stack_push(ctx, from_bytes(digest.data(), digest.size()));
                break;
            }
            case 0x50: {  // POP
                stack_pop(ctx);
                break;
            }
            case 0x51: {  // MLOAD
                const cpp_int offset_int = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t offset = 0;
                if (!to_uint64_safe(offset_int, offset)) {
                    ctx.error = "mload offset overflow";
                    ctx.halted = true;
                    break;
                }
                ensure_memory(ctx, offset, 32);
                stack_push(ctx, from_bytes(ctx.memory.data() + offset, 32));
                break;
            }
            case 0x52: {  // MSTORE
                const cpp_int offset_int = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t offset = 0;
                if (!to_uint64_safe(offset_int, offset)) {
                    ctx.error = "mstore offset overflow";
                    ctx.halted = true;
                    break;
                }
                ensure_memory(ctx, offset, 32);
                const auto bytes = to_bytes32(value);
                std::copy(bytes.begin(), bytes.end(), ctx.memory.begin() + offset);
                break;
            }
            case 0x53: {  // MSTORE8
                const cpp_int offset_int = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t offset = 0;
                if (!to_uint64_safe(offset_int, offset)) {
                    ctx.error = "mstore8 offset overflow";
                    ctx.halted = true;
                    break;
                }
                ensure_memory(ctx, offset, 1);
                ctx.memory[offset] = static_cast<uint8_t>(mask_word(value).convert_to<uint64_t>() & 0xff);
                break;
            }
            case 0x54: {  // SLOAD
                const cpp_int key = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                const auto it = ctx.storage.find(key);
                const cpp_int value = (it != ctx.storage.end()) ? it->second : cpp_int(0);
                stack_push(ctx, value);
                break;
            }
            case 0x55: {  // SSTORE
                const cpp_int key = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                ctx.storage[key] = mask_word(value);
                break;
            }
            case 0x56: {  // JUMP
                const cpp_int dest_int = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t dest = 0;
                if (!to_uint64_safe(dest_int, dest) || dest >= code_size || bytecode.bytes[dest] != 0x5b) {
                    ctx.error = "invalid jump";
                    ctx.halted = true;
                    break;
                }
                pc = dest + 1;
                break;
            }
            case 0x57: {  // JUMPI
                const cpp_int dest_int = stack_pop(ctx);
                const cpp_int condition = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                if (condition != 0) {
                    uint64_t dest = 0;
                    if (!to_uint64_safe(dest_int, dest) || dest >= code_size || bytecode.bytes[dest] != 0x5b) {
                        ctx.error = "invalid jump";
                        ctx.halted = true;
                        break;
                    }
                    pc = dest + 1;
                }
                break;
            }
            case 0x58: {  // PC
                stack_push(ctx, cpp_int(pc - 1));
                break;
            }
            case 0x5b:  // JUMPDEST
                break;
            case 0x5c: {  // TLOAD
                const cpp_int key = stack_pop(ctx);
                if (!ctx.halted) {
                    const auto it = ctx.transient_storage.find(key);
                    const cpp_int value = (it != ctx.transient_storage.end()) ? it->second : cpp_int(0);
                    stack_push(ctx, value);
                }
                break;
            }
            case 0x5d: {  // TSTORE
                const cpp_int key = stack_pop(ctx);
                const cpp_int value = stack_pop(ctx);
                if (!ctx.halted) {
                    ctx.transient_storage[key] = mask_word(value);
                }
                break;
            }
            case 0xf3: {  // RETURN
                const cpp_int offset_int = stack_pop(ctx);
                const cpp_int size_int = stack_pop(ctx);
                if (ctx.halted) {
                    break;
                }
                uint64_t offset = 0;
                uint64_t size = 0;
                if (!to_uint64_safe(offset_int, offset) || !to_uint64_safe(size_int, size)) {
                    ctx.error = "return offset overflow";
                    ctx.halted = true;
                    break;
                }
                ensure_memory(ctx, offset, size);
                ctx.return_data.assign(ctx.memory.begin() + offset, ctx.memory.begin() + offset + size);
                ctx.halted = true;
                ctx.error.clear();
                break;
            }
            case 0xf9: {  // SCHNORR_VERIFY (custom)
                if (!perform_schnorr_verify(ctx, state)) {
                    ctx.halted = true;
                }
                break;
            }
            default:
                if (opcode >= 0x60 && opcode <= 0x7f) {  // PUSH1-PUSH32
                    const size_t push_size = static_cast<size_t>(opcode - 0x5f);
                    if (pc + push_size > code_size) {
                        ctx.error = "truncated push";
                        ctx.halted = true;
                        break;
                    }
                    cpp_int value = 0;
                    for (size_t i = 0; i < push_size; ++i) {
                        value <<= 8;
                        value += bytecode.bytes[pc + i];
                    }
                    stack_push(ctx, value);
                    pc += push_size;
                } else if (opcode >= 0x80 && opcode <= 0x8f) {  // DUP
                    const size_t depth = static_cast<size_t>(opcode - 0x80);
                    if (ctx.stack.size() <= depth) {
                        ctx.error = "stack underflow";
                        ctx.halted = true;
                        break;
                    }
                    stack_push(ctx, read_stack_index(ctx, depth));
                } else if (opcode >= 0x90 && opcode <= 0x9f) {  // SWAP
                    const size_t depth = static_cast<size_t>(opcode - 0x90 + 1);
                    if (ctx.stack.size() <= depth) {
                        ctx.error = "stack underflow";
                        ctx.halted = true;
                        break;
                    }
                    const size_t idx_a = ctx.stack.size() - 1;
                    const size_t idx_b = ctx.stack.size() - 1 - depth;
                    std::swap(ctx.stack[idx_a], ctx.stack[idx_b]);
                } else {
                    ctx.error = "unsupported opcode";
                    ctx.halted = true;
                }
                break;
        }
    }

    evm_result result;
    result.success = ctx.error.empty();
    result.gas_used = ctx.gas_used;
    result.return_data = ctx.return_data;
    result.storage = ctx.storage;
    result.error = ctx.error;
    return result;
}

