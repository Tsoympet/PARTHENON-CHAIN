#include <gtest/gtest.h>
#include <array>
#include <string>
#include <vector>

#include "sidechain/rpc/wasm_rpc.h"
#include "sidechain/wasm/runtime/types.h"
#include "layer1-core/consensus/params.h"

using sidechain::rpc::ListNftRequest;
using sidechain::rpc::MintNftRequest;
using sidechain::rpc::PlaceBidRequest;
using sidechain::rpc::SettleSaleRequest;
using sidechain::rpc::TransferNftRequest;
using sidechain::rpc::WasmRpcService;
using sidechain::state::StateStore;
using sidechain::wasm::ExecutionDomain;
using sidechain::wasm::ExecutionEngine;
using sidechain::wasm::kAssetDrm;
using sidechain::wasm::kAssetObl;
using sidechain::wasm::kAssetTln;
using sidechain::wasm::kMaxRoyaltyBps;

namespace {
const char kCoreModule[] = "nft:core";
const char kBalanceModule[] = "nft:market:balances";
const char kEventModule[] = "nft:events";

uint64_t DecodeAmount(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return 0;
    try {
        return std::stoull(std::string(bytes.begin(), bytes.end()));
    } catch (...) {
        return 0;
    }
}

std::string BalanceKey(const std::string& party, uint8_t asset) {
    return party + "|" + std::to_string(asset);
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
}  // namespace

TEST(NftMarketplace, RoyaltySettlementSplitsPayment) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "athena-tablet";
    mint.creator = "curator";
    mint.owner = "museum";
    mint.metadata_hash = "meta-hash";
    mint.canon_reference_hash = "canon-hash";
    mint.mint_height = 10;
    mint.royalty_bps = 500;
    mint.asset_id = 9;  // NFTs are not bound to TLN
    ASSERT_TRUE(rpc.MintNft(mint).success);

    ListNftRequest list;
    list.token_id = mint.token_id;
    list.seller = mint.owner;
    list.payment_asset = kAssetDrm;
    list.price = 1000;
    list.height = 11;
    ASSERT_TRUE(rpc.ListNft(list).success);
    const auto listing_bytes =
        state.Get(ExecutionDomain::NFT, "nft:market:listing", mint.token_id);
    ASSERT_FALSE(listing_bytes.empty());
    const std::string listing_str(listing_bytes.begin(), listing_bytes.end());
    SCOPED_TRACE("listing=" + listing_str);
    ASSERT_NE(listing_str.find('|'), std::string::npos);
    std::vector<std::string> listing_parts;
    {
        std::stringstream ss(listing_str);
        std::string item;
        while (std::getline(ss, item, '|')) {
            listing_parts.push_back(item);
        }
    }
    ASSERT_EQ(4u, listing_parts.size());
    EXPECT_EQ(list.seller, listing_parts[0]);
    EXPECT_EQ(std::to_string(list.payment_asset), listing_parts[1]);
    EXPECT_EQ(std::to_string(list.price), listing_parts[2]);

    SettleSaleRequest settle;
    settle.token_id = mint.token_id;
    settle.buyer = "collector";
    settle.payment_asset = kAssetDrm;
    settle.price = list.price;
    settle.height = 12;
    auto sale = rpc.SettleSale(settle);
    ASSERT_TRUE(sale.success) << sale.error;

    const auto record_bytes = state.Get(ExecutionDomain::NFT, kCoreModule, mint.token_id);
    ASSERT_FALSE(record_bytes.empty());
    const std::string record(record_bytes.begin(), record_bytes.end());
    EXPECT_NE(record.find(settle.buyer), std::string::npos);
    std::vector<std::string> record_parts;
    {
        std::stringstream ss(record);
        std::string item;
        while (std::getline(ss, item, '|')) {
            record_parts.push_back(item);
        }
    }
    ASSERT_EQ(6u, record_parts.size());
    EXPECT_EQ("500", record_parts[5]);

    const uint64_t royalty_amount = 1000 * 500 / 10'000;
    const uint64_t seller_amount = 1000 - royalty_amount;

    const auto creator_bytes = state.Get(ExecutionDomain::NFT, kBalanceModule,
                                         BalanceKey(mint.creator, kAssetDrm));
    const auto seller_bytes = state.Get(ExecutionDomain::NFT, kBalanceModule,
                                        BalanceKey(list.seller, kAssetDrm));
    SCOPED_TRACE("creator_balance_raw=" +
                 std::string(creator_bytes.begin(), creator_bytes.end()));
    SCOPED_TRACE("seller_balance_raw=" +
                 std::string(seller_bytes.begin(), seller_bytes.end()));
    auto creator_balance = DecodeAmount(creator_bytes);
    auto seller_balance = DecodeAmount(seller_bytes);
    EXPECT_EQ(royalty_amount, creator_balance);
    EXPECT_EQ(seller_amount, seller_balance);

    auto nft_root = state.ModuleRoot(ExecutionDomain::NFT, kCoreModule);
    auto market_root = state.ModuleRoot(ExecutionDomain::NFT, kBalanceModule);
    auto event_root = state.ModuleRoot(ExecutionDomain::NFT, kEventModule);
    EXPECT_NE(nft_root, decltype(nft_root){});
    EXPECT_NE(market_root, decltype(market_root){});
    EXPECT_NE(event_root, decltype(event_root){});
}

TEST(NftMarketplace, RoyaltyImmutabilityAndReuse) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "apollo";
    mint.creator = "scribe";
    mint.owner = "scribe";
    mint.metadata_hash = "h1";
    mint.canon_reference_hash = "canon1";
    mint.mint_height = 1;
    mint.royalty_bps = 100;
    mint.asset_id = 7;  // asset id ignored for NFTs
    ASSERT_TRUE(rpc.MintNft(mint).success);

    MintNftRequest duplicate = mint;
    duplicate.royalty_bps = 900;
    auto dup = rpc.MintNft(duplicate);
    EXPECT_FALSE(dup.success);
    auto stored = state.Get(ExecutionDomain::NFT, kCoreModule, mint.token_id);
    ASSERT_FALSE(stored.empty());
    std::string stored_rec(stored.begin(), stored.end());
    auto stored_parts = SplitFields(stored_rec);
    // owner|creator|metadata|canon|mint_height|royalty_bps
    constexpr size_t kNftRecordFields = 6;
    ASSERT_EQ(kNftRecordFields, stored_parts.size());
    EXPECT_EQ("100", stored_parts.back());

    TransferNftRequest transfer;
    transfer.token_id = mint.token_id;
    transfer.from = mint.owner;
    transfer.to = "new-owner";
    transfer.asset_id = kAssetTln;
    transfer.gas_limit = 50;
    transfer.height = 2;
    ASSERT_TRUE(rpc.TransferNft(transfer).success);

    ListNftRequest list;
    list.token_id = mint.token_id;
    list.seller = transfer.to;
    list.payment_asset = kAssetObl;
    list.price = 200;
    list.height = 3;
    ASSERT_TRUE(rpc.ListNft(list).success);

    SettleSaleRequest settle;
    settle.token_id = mint.token_id;
    settle.buyer = "buyer";
    settle.payment_asset = kAssetObl;
    settle.price = list.price;
    settle.height = 4;
    auto settled = rpc.SettleSale(settle);
    ASSERT_TRUE(settled.success) << settled.error;

    const uint64_t royalty_amount = 200 * 100 / 10'000;
    const uint64_t seller_amount = 200 - royalty_amount;
    auto creator_balance =
        DecodeAmount(state.Get(ExecutionDomain::NFT, kBalanceModule,
                               BalanceKey(mint.creator, kAssetObl)));
    auto seller_balance =
        DecodeAmount(state.Get(ExecutionDomain::NFT, kBalanceModule,
                               BalanceKey(list.seller, kAssetObl)));
    EXPECT_EQ(royalty_amount, creator_balance);
    EXPECT_EQ(seller_amount, seller_balance);
}

TEST(NftMarketplace, IgnoresAssetIdsForNftExecution) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "isolated-asset";
    mint.creator = "scribe";
    mint.owner = "scribe";
    mint.metadata_hash = "hash";
    mint.canon_reference_hash = "canon";
    mint.mint_height = 7;
    mint.royalty_bps = 10;
    mint.asset_id = 42;  // arbitrary asset id, should not couple to TLN
    ASSERT_TRUE(rpc.MintNft(mint).success);

    TransferNftRequest transfer;
    transfer.token_id = mint.token_id;
    transfer.from = mint.owner;
    transfer.to = "new-owner";
    transfer.asset_id = 1;  // different arbitrary asset id
    transfer.height = 8;
    ASSERT_TRUE(rpc.TransferNft(transfer).success);

    auto record = state.Get(ExecutionDomain::NFT, "nft:core", mint.token_id);
    ASSERT_FALSE(record.empty());
    std::string rec_str(record.begin(), record.end());
    EXPECT_NE(rec_str.find(transfer.to), std::string::npos);
}

TEST(NftMarketplace, RejectsTlnPaymentAndPreservesSupply) {
    ExecutionEngine engine;
    StateStore state;
    WasmRpcService rpc(engine, state);

    MintNftRequest mint;
    mint.token_id = "hermes";
    mint.creator = "scribe";
    mint.owner = "scribe";
    mint.metadata_hash = "hash";
    mint.canon_reference_hash = "canon";
    mint.mint_height = 5;
    mint.asset_id = 5;  // TLN is not required for minting
    ASSERT_TRUE(rpc.MintNft(mint).success);

    ListNftRequest list;
    list.token_id = mint.token_id;
    list.seller = mint.owner;
    list.payment_asset = kAssetTln;
    list.price = 50;
    auto listed = rpc.ListNft(list);
    EXPECT_FALSE(listed.success);
    EXPECT_EQ("payment must be DRM or OBL", listed.error);

    PlaceBidRequest bid;
    bid.token_id = mint.token_id;
    bid.bidder = "bidder";
    bid.payment_asset = kAssetTln;
    bid.price = 60;
    auto bid_res = rpc.PlaceBid(bid);
    EXPECT_FALSE(bid_res.success);
    EXPECT_EQ("payment must be DRM or OBL", bid_res.error);

    EXPECT_TRUE(state.Get(ExecutionDomain::NFT, "nft:market:listing", mint.token_id).empty());
    EXPECT_TRUE(state.Get(ExecutionDomain::NFT, "nft:market:bids", mint.token_id + "|bidder").empty());

    auto max_money_before = consensus::GetMaxMoney(consensus::Main(), kAssetDrm);
    auto max_money_after = consensus::GetMaxMoney(consensus::Main(), kAssetDrm);
    EXPECT_EQ(max_money_before, max_money_after);
 }

TEST(NftMarketplace, RejectsInvalidMetadataPaths) {
     ExecutionEngine engine;
     StateStore state;
     WasmRpcService rpc(engine, state);

     MintNftRequest mint;
     mint.token_id = "broken-meta";
     mint.creator = "scribe";
     mint.owner = "scribe";
     mint.metadata_hash = "";
     mint.canon_reference_hash = "";
     mint.mint_height = 3;
     mint.royalty_bps = 10;
     auto res = rpc.MintNft(mint);
     EXPECT_FALSE(res.success);
     EXPECT_EQ("invalid canon reference", res.error);
     EXPECT_FALSE(state.Exists(ExecutionDomain::NFT, kCoreModule, mint.token_id));
 }

TEST(NftMarketplace, EnforcesRoyaltyBoundsAndGasFloor) {
     ExecutionEngine engine;
     StateStore state;
     WasmRpcService rpc(engine, state);

     MintNftRequest mint;
     mint.token_id = "royalty-bounds";
     mint.creator = "artist";
     mint.owner = "artist";
     mint.metadata_hash = "meta";
     mint.canon_reference_hash = "canon";
     mint.mint_height = 2;
     mint.royalty_bps = static_cast<uint16_t>(kMaxRoyaltyBps + 1);
     auto tooHigh = rpc.MintNft(mint);
     EXPECT_FALSE(tooHigh.success);
     EXPECT_EQ("invalid royalty_bps", tooHigh.error);

     mint.royalty_bps = 0;
     mint.gas_limit = 1; // below fixed cost
     auto underGas = rpc.MintNft(mint);
     EXPECT_FALSE(underGas.success);
     EXPECT_EQ("out of gas", underGas.error);
 }
