#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sidechain::wasm {

// Assets: TLN = 0, DRM = 1, OBL = 2
constexpr uint8_t kAssetTln = 0;
constexpr uint8_t kAssetDrm = 1;
constexpr uint8_t kAssetObl = 2;

enum class ExecutionDomain {
    SmartContract,
    NFT,
    Dapp,
};

enum class OpCode {
    Nop,
    ConstI32,
    AddI32,
    Load,
    Store,
    ReturnTop,
};

struct Instruction {
    OpCode op{OpCode::Nop};
    int32_t immediate{0};
};

struct ExecutionRequest {
    ExecutionDomain domain{ExecutionDomain::SmartContract};
    uint8_t asset_id{kAssetDrm};
    std::string module_id;
    std::vector<Instruction> code;
    uint64_t gas_limit{0};
    std::vector<uint8_t> input;
};

struct ExecutionResult {
    bool success{false};
    uint64_t gas_used{0};
    std::vector<uint8_t> output;
    std::string error;
    uint32_t state_writes{0};
};

}  // namespace sidechain::wasm
