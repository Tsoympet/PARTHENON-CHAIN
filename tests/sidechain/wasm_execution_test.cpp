#include <gtest/gtest.h>

#include <cstring>

#include "sidechain/rpc/wasm_rpc.h"
#include "sidechain/wasm/runtime/engine.h"

using sidechain::rpc::DappCall;
using sidechain::rpc::MintNftRequest;
using sidechain::rpc::WasmRpcService;
using sidechain::state::StateStore;
using sidechain::wasm::ExecutionDomain;
using sidechain::wasm::ExecutionEngine;
using sidechain::wasm::ExecutionRequest;
using sidechain::wasm::Instruction;
using sidechain::wasm::OpCode;
using sidechain::wasm::kAssetDrm;
using sidechain::wasm::kAssetObl;
using sidechain::wasm::kAssetTln;

TEST(WasmAssetLaw, RejectsMismatchedAsset) {
    ExecutionEngine engine;
    StateStore state;
    ExecutionRequest req;
    req.domain = ExecutionDomain::SmartContract;
    req.asset_id = kAssetTln;
    req.module_id = "contract";
    req.code = {Instruction{OpCode::ConstI32, 1}};
    req.gas_limit = 50;

    auto res = engine.Execute(req, state);
    EXPECT_FALSE(res.success);
    EXPECT_EQ("asset/domain violation", res.error);
}

TEST(WasmAssetLaw, NftRequiresTln) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest bad;
    bad.token_id = "token-1";
    bad.creator = "alice";
    bad.owner = "alice";
    bad.metadata_hash = "hash";
    bad.canon_reference_hash = "canon";
    bad.mint_height = 1;
    bad.royalty_bps = 0;
    bad.asset_id = kAssetDrm;
    auto rejected = rpc.MintNft(bad);
    EXPECT_FALSE(rejected.success);
    EXPECT_EQ("asset/domain violation", rejected.error);

    bad.asset_id = kAssetTln;
    auto accepted = rpc.MintNft(bad);
    EXPECT_TRUE(accepted.success);
}

TEST(WasmDeterminism, RepeatableGasAndOutput) {
    ExecutionEngine engine;
    StateStore state;

    std::vector<Instruction> code = {
        {OpCode::ConstI32, 5},
        {OpCode::ConstI32, 7},
        {OpCode::AddI32, 0},
        {OpCode::ReturnTop, 0},
    };

    ExecutionRequest req;
    req.domain = ExecutionDomain::SmartContract;
    req.asset_id = kAssetDrm;
    req.module_id = "adder";
    req.code = code;
    req.gas_limit = 100;

    auto first = engine.Execute(req, state);
    auto second = engine.Execute(req, state);

    ASSERT_TRUE(first.success);
    ASSERT_TRUE(second.success);
    EXPECT_EQ(first.gas_used, second.gas_used);
    EXPECT_EQ(first.output, second.output);

    uint32_t value = 0;
    std::memcpy(&value, first.output.data(), sizeof(value));
    EXPECT_EQ(12u, value);
}

TEST(WasmSafety, StackLimitEnforced) {
    ExecutionEngine engine;
    StateStore state;
    std::vector<Instruction> code(1100, Instruction{OpCode::ConstI32, 1});

    ExecutionRequest req;
    req.domain = ExecutionDomain::Dapp;
    req.asset_id = kAssetObl;
    req.module_id = "stack-test";
    req.code = code;
    req.gas_limit = 100000;

    auto res = engine.Execute(req, state);
    EXPECT_FALSE(res.success);
    EXPECT_EQ("stack limit exceeded", res.error);
}
