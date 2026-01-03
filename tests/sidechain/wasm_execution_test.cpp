#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <functional>
#include <sstream>
#include <chrono>
#include <random>
#include <iomanip>

#include "sidechain/rpc/wasm_rpc.h"
#include "sidechain/wasm/runtime/engine.h"
#include "sidechain/wasm/validator/validator.h"
#include "layer1-core/chainstate/coins.h"
#include "layer1-core/consensus/params.h"

using sidechain::rpc::DappCall;
using sidechain::rpc::ListNftRequest;
using sidechain::rpc::MintNftRequest;
using sidechain::rpc::SettleSaleRequest;
using sidechain::rpc::TransferNftRequest;
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

namespace {
// Well-known hash mixing constant derived from the golden ratio fraction.
constexpr std::size_t kHashMagic = 0x9e3779b97f4a7c15ULL;
constexpr std::size_t kTestCacheEntries = 8;

std::size_t DigestUtxos(const Chainstate& chain,
                        const std::vector<OutPoint>& outs) {
    std::hash<std::string> h;
    std::size_t acc = 0;
    for (const auto& op : outs) {
        auto utxo = chain.GetUTXO(op);
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (auto b : op.hash) {
            oss << std::setw(2) << static_cast<int>(b);
        }
        oss << std::dec << ":" << op.index << ":" << static_cast<int>(utxo.assetId) << ":"
            << utxo.value;
        acc ^= h(oss.str()) + kHashMagic + (acc << 6) + (acc >> 2);
    }
    return acc;
}

std::vector<std::string> SplitFields(const std::string& s) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }
    return parts;
}

struct TempPathGuard {
    explicit TempPathGuard(std::string base) : base_path(std::move(base)) {
        std::filesystem::remove_all(base_path);
        std::filesystem::remove_all(base_path + ".ldb");
    }
    ~TempPathGuard() {
        std::filesystem::remove_all(base_path);
        std::filesystem::remove_all(base_path + ".ldb");
    }
    std::string base_path;
};
}  // namespace

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

TEST(WasmAssetLaw, NftAssetAgnostic) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest drm;
    drm.token_id = "token-1";
    drm.creator = "alice";
    drm.owner = "alice";
    drm.metadata_hash = "hash";
    drm.canon_reference_hash = "canon";
    drm.mint_height = 1;
    drm.royalty_bps = 0;
    drm.asset_id = kAssetDrm;
    auto minted_drm = rpc.MintNft(drm);
    EXPECT_TRUE(minted_drm.success);

    MintNftRequest tln = drm;
    tln.token_id = "token-2";
    tln.asset_id = kAssetTln;
    auto minted_tln = rpc.MintNft(tln);
    EXPECT_TRUE(minted_tln.success);

    auto root_drm = state.ModuleRoot(ExecutionDomain::NFT, "nft:core");
    EXPECT_NE(root_drm, decltype(root_drm){});
    auto record = state.Get(ExecutionDomain::NFT, "nft:core", tln.token_id);
    ASSERT_FALSE(record.empty());
    std::string rec_str(record.begin(), record.end());
    auto parts = SplitFields(rec_str);
    ASSERT_EQ(6u, parts.size());
    EXPECT_EQ(parts[0], tln.owner);
    EXPECT_EQ(parts[1], tln.creator);
    EXPECT_EQ(parts[2], tln.metadata_hash);
    EXPECT_EQ(parts[3], tln.canon_reference_hash);
    EXPECT_EQ(parts[4], std::to_string(tln.mint_height));
    EXPECT_EQ(parts[5], std::to_string(tln.royalty_bps));
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

TEST(WasmState, MintCreatesOnlyCoreEntry) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "core-only";
    mint.creator = "scribe";
    mint.owner = "scribe";
    mint.metadata_hash = "meta";
    mint.canon_reference_hash = "canon";
    mint.mint_height = 5;
    mint.royalty_bps = 50;
    mint.asset_id = kAssetObl;
    auto res = rpc.MintNft(mint);
    ASSERT_TRUE(res.success);

    auto core_root = state.ModuleRoot(ExecutionDomain::NFT, "nft:core");
    EXPECT_NE(core_root, decltype(core_root){});
    EXPECT_EQ(state.ModuleRoot(ExecutionDomain::NFT, "nft:market:balances"),
              decltype(core_root){});
    EXPECT_EQ(state.ModuleRoot(ExecutionDomain::NFT, "nft:market:listing"),
              decltype(core_root){});
    EXPECT_EQ(state.ModuleRoot(ExecutionDomain::NFT, "nft:market:bids"),
              decltype(core_root){});

    auto record = state.Get(ExecutionDomain::NFT, "nft:core", mint.token_id);
    ASSERT_FALSE(record.empty());
    std::string rec_str(record.begin(), record.end());
    EXPECT_NE(rec_str.find(mint.owner), std::string::npos);
    EXPECT_NE(rec_str.find(std::to_string(mint.royalty_bps)), std::string::npos);
}

TEST(WasmIsolation, Layer1UntouchedByNftLifecycle) {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    const auto unique_suffix = static_cast<unsigned long long>(dist(rd));
    auto tmp = (std::filesystem::temp_directory_path() /
                ("drachma_utxo_isolation-" + std::to_string(unique_suffix)))
                   .string();
    TempPathGuard guard(tmp);

    Chainstate chain(guard.base_path, kTestCacheEntries);
    OutPoint op1{};
    op1.hash.fill(0xAA);
    op1.index = 0;
    TxOut tx1{1000, {0x51}, static_cast<uint8_t>(AssetId::TALANTON)};
    chain.AddUTXO(op1, tx1);

    OutPoint op2{};
    op2.hash.fill(0xBB);
    op2.index = 1;
    TxOut tx2{500, {0x52}, static_cast<uint8_t>(AssetId::DRACHMA)};
    chain.AddUTXO(op2, tx2);
    chain.Flush();

    const std::vector<OutPoint> outs{op1, op2};
    const auto digest_before = DigestUtxos(chain, outs);
    const auto cache_before = chain.CachedEntries();
    const auto tln_max_before =
        consensus::GetMaxMoney(consensus::Main(),
                               static_cast<uint8_t>(AssetId::TALANTON));

    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "isolation-nft";
    mint.creator = "curator";
    mint.owner = "curator";
    mint.metadata_hash = "meta";
    mint.canon_reference_hash = "canon";
    mint.mint_height = 10;
    mint.royalty_bps = 250;
    mint.asset_id = kAssetObl;
    ASSERT_TRUE(rpc.MintNft(mint).success);
    auto record_before = state.Get(ExecutionDomain::NFT, "nft:core", mint.token_id);
    ASSERT_FALSE(record_before.empty());

    TransferNftRequest transfer;
    transfer.token_id = mint.token_id;
    transfer.from = mint.owner;
    transfer.to = "new-owner";
    transfer.asset_id = kAssetObl;
    transfer.height = 11;
    ASSERT_TRUE(rpc.TransferNft(transfer).success);

    ListNftRequest list;
    list.token_id = mint.token_id;
    list.seller = transfer.to;
    list.payment_asset = kAssetDrm;
    list.price = 700;
    list.height = 12;
    ASSERT_TRUE(rpc.ListNft(list).success);

    SettleSaleRequest settle;
    settle.token_id = mint.token_id;
    settle.buyer = "collector";
    settle.payment_asset = kAssetDrm;
    settle.price = list.price;
    settle.height = 13;
    auto sale = rpc.SettleSale(settle);
    ASSERT_TRUE(sale.success);

    auto record_after = state.Get(ExecutionDomain::NFT, "nft:core", mint.token_id);
    ASSERT_FALSE(record_after.empty());
    std::string after_str(record_after.begin(), record_after.end());
    auto parts = SplitFields(after_str);
    ASSERT_EQ(6u, parts.size());
    EXPECT_EQ(parts[0], settle.buyer);
    EXPECT_EQ(parts[1], mint.creator);
    EXPECT_EQ(parts[2], mint.metadata_hash);
    EXPECT_EQ(parts[3], mint.canon_reference_hash);
    EXPECT_EQ(parts[4], std::to_string(mint.mint_height));
    EXPECT_EQ(parts[5], std::to_string(mint.royalty_bps));

    const auto digest_after = DigestUtxos(chain, outs);
    EXPECT_EQ(cache_before, chain.CachedEntries());
    EXPECT_EQ(digest_before, digest_after);
    EXPECT_TRUE(chain.HaveUTXO(op1));
    EXPECT_TRUE(chain.HaveUTXO(op2));

    const auto tln_max_after =
        consensus::GetMaxMoney(consensus::Main(),
                               static_cast<uint8_t>(AssetId::TALANTON));
    EXPECT_EQ(tln_max_before, tln_max_after);
}

TEST(WasmCheckpoint, RejectsInvalidAnchors) {
    sidechain::wasm::SidechainBlockHeader header{};
    header.main_chain_checkpoint.fill(0xAA);
    header.state_root.fill(0x01);
    header.execution_root.fill(0x02);
    header.market_state_root.fill(0x03);
    header.event_root.fill(0x04);

    std::string error;
    std::array<uint8_t, 32> expected = header.main_chain_checkpoint;
    EXPECT_FALSE(sidechain::wasm::ValidateCheckpoint(header, expected, error));
    EXPECT_EQ("missing execution anchors", error);

    header.nft_state_root.fill(0x05);
    error.clear();
    expected.fill(0x0F);
    EXPECT_FALSE(sidechain::wasm::ValidateCheckpoint(header, expected, error));
    EXPECT_EQ("checkpoint mismatch", error);

    error.clear();
    expected = header.main_chain_checkpoint;
    EXPECT_TRUE(sidechain::wasm::ValidateCheckpoint(header, expected, error));
    EXPECT_TRUE(error.empty());
}

TEST(WasmStateStore, DeterministicRoots)
{
    StateStore state;
    auto emptyDomain = state.DomainRoot(ExecutionDomain::NFT);
    EXPECT_EQ(emptyDomain, decltype(emptyDomain){});

    state.Put(ExecutionDomain::NFT, "module", "b", {0x01});
    state.Put(ExecutionDomain::NFT, "module", "a", {0x02});
    auto first = state.ModuleRoot(ExecutionDomain::NFT, "module");

    // Reinsert in different order to ensure deterministic hashing.
    state.Put(ExecutionDomain::NFT, "module", "a", {0x02});
    state.Put(ExecutionDomain::NFT, "module", "b", {0x01});
    auto second = state.ModuleRoot(ExecutionDomain::NFT, "module");
    EXPECT_EQ(first, second);

    auto domainRoot = state.DomainRoot(ExecutionDomain::NFT);
    EXPECT_NE(domainRoot, decltype(domainRoot){});
}
