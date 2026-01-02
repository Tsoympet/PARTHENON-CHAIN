#include "wasm_rpc.h"

#include "../wasm/validator/validator.h"

namespace sidechain::rpc {

using sidechain::wasm::ExecutionDomain;
using sidechain::wasm::ExecutionRequest;
using sidechain::wasm::ExecutionResult;
using sidechain::wasm::Instruction;

namespace {
constexpr uint64_t kFixedNftGas = 50;
constexpr char kNftModule[] = "nft";
constexpr char kNftMetaModule[] = "nft:meta";

bool Validate(ExecutionDomain domain, uint8_t asset, std::string& error) {
    return sidechain::wasm::ValidateAssetDomain({domain, asset}, error);
}

bool ApplyFixedNftCost(uint64_t provided_limit, ExecutionResult& res) {
    const uint64_t limit = provided_limit == 0 ? kFixedNftGas : provided_limit;
    if (limit < kFixedNftGas) {
        res.error = "out of gas";
        res.gas_used = limit;
        return false;
    }
    res.gas_used = kFixedNftGas;
    return true;
}
}  // namespace

WasmRpcService::WasmRpcService(sidechain::wasm::ExecutionEngine& engine,
                               sidechain::state::StateStore& state)
    : engine_(engine), state_(state) {}

ExecutionResult WasmRpcService::DeployContract(const DeployRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::SmartContract, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }

    ExecutionRequest exec;
    exec.domain = ExecutionDomain::SmartContract;
    exec.asset_id = request.asset_id;
    exec.module_id = request.contract_id;
    exec.code = request.init_code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

ExecutionResult WasmRpcService::CallContract(const ContractCall& request) {
    std::string error;
    if (!Validate(ExecutionDomain::SmartContract, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }

    ExecutionRequest exec;
    exec.domain = ExecutionDomain::SmartContract;
    exec.asset_id = request.asset_id;
    exec.module_id = request.contract_id;
    exec.code = request.code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

ExecutionResult WasmRpcService::MintNft(const MintNftRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::NFT, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionResult res;
    if (!ApplyFixedNftCost(request.gas_limit, res)) {
        return res;
    }
    if (state_.Exists(ExecutionDomain::NFT, kNftModule, request.token_id)) {
        res.error = "token exists";
        return res;
    }
    std::vector<uint8_t> owner_bytes(request.owner.begin(), request.owner.end());
    std::vector<uint8_t> meta_bytes(request.metadata_hash.begin(), request.metadata_hash.end());
    state_.Put(ExecutionDomain::NFT, kNftModule, request.token_id, owner_bytes);
    state_.Put(ExecutionDomain::NFT, kNftMetaModule, request.token_id, meta_bytes);
    res.success = true;
    return res;
}

ExecutionResult WasmRpcService::TransferNft(const TransferNftRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::NFT, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionResult res;
    if (!ApplyFixedNftCost(request.gas_limit, res)) {
        return res;
    }
    if (!state_.Exists(ExecutionDomain::NFT, kNftModule, request.token_id)) {
        res.error = "token missing";
        return res;
    }
    const auto owner_bytes = state_.Get(ExecutionDomain::NFT, kNftModule, request.token_id);
    const std::string current_owner(owner_bytes.begin(), owner_bytes.end());
    if (current_owner != request.from) {
        res.error = "ownership mismatch";
        return res;
    }
    std::vector<uint8_t> new_owner(request.to.begin(), request.to.end());
    state_.Put(ExecutionDomain::NFT, kNftModule, request.token_id, new_owner);
    res.success = true;
    return res;
}

ExecutionResult WasmRpcService::CallDapp(const DappCall& request) {
    std::string error;
    if (!Validate(ExecutionDomain::Dapp, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionRequest exec;
    exec.domain = ExecutionDomain::Dapp;
    exec.asset_id = request.asset_id;
    exec.module_id = request.app_id;
    exec.code = request.code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

}  // namespace sidechain::rpc
