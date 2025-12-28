#include "sidechain/contracts/precompiles/nft.h"

#include <filesystem>
#include <gtest/gtest.h>

using sidechain::precompiles::address;
using sidechain::precompiles::nft_precompile;
using sidechain::precompiles::nft_result;
using sidechain::precompiles::uint256;

namespace {
address make_address(uint8_t seed) {
    address addr{};
    for (size_t i = 0; i < addr.size(); ++i) {
        addr[i] = static_cast<uint8_t>(seed + i);
    }
    return addr;
}

std::string temp_db_path(const std::string& name) {
    auto base = std::filesystem::temp_directory_path();
    auto path = base / name;
    std::filesystem::remove_all(path);
    return path.string();
}
}  // namespace

TEST(NftPrecompileTest, MintAndOwnerLookup) {
    const std::string db_path = temp_db_path("nft_mint_lookup");
    nft_precompile precompile(db_path);

    const uint256 token_id = 42;
    const address owner = make_address(0x01);

    nft_result minted = precompile.mint(token_id, owner, "ipfs://token/42");
    EXPECT_TRUE(minted.success);
    ASSERT_TRUE(minted.owner.has_value());
    EXPECT_EQ(minted.owner.value(), owner);
    ASSERT_TRUE(minted.metadata_uri.has_value());
    EXPECT_EQ(minted.metadata_uri.value(), "ipfs://token/42");

    nft_result queried = precompile.owner_of(token_id);
    EXPECT_TRUE(queried.success);
    ASSERT_TRUE(queried.owner.has_value());
    EXPECT_EQ(queried.owner.value(), owner);
}

TEST(NftPrecompileTest, TransferUpdatesOwner) {
    const std::string db_path = temp_db_path("nft_transfer");
    nft_precompile precompile(db_path);

    const uint256 token_id = 7;
    const address minter = make_address(0x02);
    const address recipient = make_address(0x10);

    ASSERT_TRUE(precompile.mint(token_id, minter, "ipfs://token/7").success);

    nft_result unauthorized = precompile.transfer(recipient, minter, token_id);
    EXPECT_FALSE(unauthorized.success);
    EXPECT_FALSE(unauthorized.owner.has_value());

    nft_result transfer_res = precompile.transfer(minter, recipient, token_id);
    EXPECT_TRUE(transfer_res.success);
    ASSERT_TRUE(transfer_res.owner.has_value());
    EXPECT_EQ(transfer_res.owner.value(), recipient);

    nft_result queried = precompile.owner_of(token_id);
    EXPECT_TRUE(queried.success);
    ASSERT_TRUE(queried.owner.has_value());
    EXPECT_EQ(queried.owner.value(), recipient);
}

TEST(NftPrecompileTest, MintFailsForExistingToken) {
    const std::string db_path = temp_db_path("nft_duplicate_mint");
    nft_precompile precompile(db_path);

    const uint256 token_id = 9999;
    const address owner = make_address(0x05);

    EXPECT_TRUE(precompile.mint(token_id, owner, "").success);

    nft_result second = precompile.mint(token_id, owner, "");
    EXPECT_FALSE(second.success);
    EXPECT_FALSE(second.owner.has_value());
}

TEST(NftPrecompileTest, TokenUriAndApprovals) {
    const std::string db_path = temp_db_path("nft_metadata");
    nft_precompile precompile(db_path);

    const uint256 token_id = 55;
    const address owner = make_address(0x0a);
    const address operator_addr = make_address(0x0b);
    const address recipient = make_address(0x0c);

    ASSERT_TRUE(precompile.mint(token_id, owner, "ipfs://token/55.json").success);

    nft_result uri_res = precompile.token_uri(token_id);
    EXPECT_TRUE(uri_res.success);
    ASSERT_TRUE(uri_res.metadata_uri.has_value());
    EXPECT_EQ(uri_res.metadata_uri.value(), "ipfs://token/55.json");

    nft_result approve_all = precompile.set_approval_for_all(owner, operator_addr, true);
    EXPECT_TRUE(approve_all.success);
    EXPECT_TRUE(approve_all.approved);

    nft_result transfer_res = precompile.transfer(operator_addr, recipient, token_id);
    EXPECT_TRUE(transfer_res.success);
    ASSERT_TRUE(transfer_res.owner.has_value());
    EXPECT_EQ(transfer_res.owner.value(), recipient);
}

