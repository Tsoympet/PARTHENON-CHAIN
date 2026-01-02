#pragma once

#include <string>
#include <vector>

#include "../state/state_store.h"
#include "../wasm/runtime/engine.h"

namespace sidechain::rpc {

struct DeployRequest {
    std::string contract_id;
    std::vector<sidechain::wasm::Instruction> init_code;
    uint8_t asset_id;
    uint64_t gas_limit{0};
};

struct ContractCall {
    std::string contract_id;
    std::vector<sidechain::wasm::Instruction> code;
    uint8_t asset_id;
    uint64_t gas_limit{0};
};

struct MintNftRequest {
    std::string token_id;
    std::string owner;
    std::string metadata_hash;
    uint8_t asset_id;
    uint64_t gas_limit{0};
};

struct TransferNftRequest {
    std::string token_id;
    std::string from;
    std::string to;
    uint8_t asset_id;
    uint64_t gas_limit{0};
};

struct DappCall {
    std::string app_id;
    std::vector<sidechain::wasm::Instruction> code;
    uint8_t asset_id;
    uint64_t gas_limit{0};
};

class WasmRpcService {
public:
    WasmRpcService(sidechain::wasm::ExecutionEngine& engine, sidechain::state::StateStore& state);

    sidechain::wasm::ExecutionResult DeployContract(const DeployRequest& request);
    sidechain::wasm::ExecutionResult CallContract(const ContractCall& request);
    sidechain::wasm::ExecutionResult MintNft(const MintNftRequest& request);
    sidechain::wasm::ExecutionResult TransferNft(const TransferNftRequest& request);
    sidechain::wasm::ExecutionResult CallDapp(const DappCall& request);

private:
    sidechain::wasm::ExecutionEngine& engine_;
    sidechain::state::StateStore& state_;
};

}  // namespace sidechain::rpc
