#include "nft.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace sidechain::precompiles {
namespace {
constexpr uint64_t k_gas_mint = 50'000;
constexpr uint64_t k_gas_transfer = 25'000;
constexpr uint64_t k_gas_owner_of = 5'000;
constexpr uint64_t k_gas_metadata = 2'000;
constexpr uint64_t k_gas_approval = 8'000;

std::array<uint8_t, 32> to_bytes32(const uint256& value) {
    std::array<uint8_t, 32> out{};
    uint256 tmp = value;
    for (int i = 31; i >= 0; --i) {
        out[static_cast<size_t>(i)] = static_cast<uint8_t>((tmp & 0xff).convert_to<uint64_t>());
        tmp >>= 8;
    }
    return out;
}

uint256 mask_word(const uint256& value) {
    return value & ((uint256(1) << 256) - 1);
}

bool addresses_equal(const address& a, const address& b) {
    return std::equal(a.begin(), a.end(), b.begin());
}

}  // namespace

nft_precompile::nft_precompile(const std::string& db_path) {
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::DB* raw_db = nullptr;
    auto status = leveldb::DB::Open(opts, db_path, &raw_db);
    if (!status.ok()) {
        throw std::runtime_error("failed to open NFT precompile DB: " + status.ToString());
    }
    db_.reset(raw_db);
}

std::string nft_precompile::encode_token_key(const uint256& token_id) const {
    const auto bytes = to_bytes32(mask_word(token_id));
    std::string key = "nft:owner:";
    key.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return key;
}

std::string nft_precompile::encode_metadata_key(const uint256& token_id) const {
    const auto bytes = to_bytes32(mask_word(token_id));
    std::string key = "nft:meta:";
    key.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return key;
}

std::string nft_precompile::encode_operator_key(const address& owner, const address& operator_addr) const {
    std::string key = "nft:op:";
    key.append(encode_address(owner));
    key.push_back(':');
    key.append(encode_address(operator_addr));
    return key;
}

std::string nft_precompile::encode_token_approval_key(const uint256& token_id) const {
    const auto bytes = to_bytes32(mask_word(token_id));
    std::string key = "nft:approval:";
    key.append(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return key;
}

std::string nft_precompile::encode_address(const address& addr) {
    return std::string(reinterpret_cast<const char*>(addr.data()), addr.size());
}

bool nft_precompile::decode_address(const std::string& value, address& out) {
    if (value.size() != out.size()) {
        return false;
    }
    std::copy(value.begin(), value.end(), out.begin());
    return true;
}

nft_result nft_precompile::mint(const uint256& token_id, const address& to, const std::string& metadata_uri) {
    nft_result result{};
    result.gas_used = k_gas_mint;

    const std::string token_key = encode_token_key(token_id);
    std::string existing;
    auto status = db_->Get(leveldb::ReadOptions(), token_key, &existing);
    if (status.ok()) {
        result.error = "token already minted";
        return result;
    }

    leveldb::WriteOptions write_opts;
    status = db_->Put(write_opts, token_key, encode_address(to));
    if (!status.ok()) {
        result.error = status.ToString();
        return result;
    }

    if (!metadata_uri.empty()) {
        status = db_->Put(write_opts, encode_metadata_key(token_id), metadata_uri);
        if (!status.ok()) {
            result.error = status.ToString();
            return result;
        }
    }

    result.success = true;
    result.owner = to;
    result.metadata_uri = metadata_uri;
    return result;
}

nft_result nft_precompile::burn(const address& owner, const uint256& token_id) {
    nft_result result{};
    result.gas_used = k_gas_transfer;

    const std::string token_key = encode_token_key(token_id);
    std::string stored_owner;
    auto status = db_->Get(leveldb::ReadOptions(), token_key, &stored_owner);
    if (!status.ok()) {
        result.error = "token not minted";
        return result;
    }

    address current_owner{};
    if (!decode_address(stored_owner, current_owner) || !addresses_equal(current_owner, owner)) {
        result.error = "burn not authorized";
        return result;
    }

    leveldb::WriteOptions write_opts;
    status = db_->Delete(write_opts, token_key);
    if (!status.ok()) {
        result.error = status.ToString();
        return result;
    }
    db_->Delete(write_opts, encode_metadata_key(token_id));
    db_->Delete(write_opts, encode_token_approval_key(token_id));

    result.success = true;
    return result;
}

nft_result nft_precompile::transfer(const address& from, const address& to, const uint256& token_id) {
    nft_result result{};
    result.gas_used = k_gas_transfer;

    const std::string token_key = encode_token_key(token_id);
    std::string stored_owner;
    auto status = db_->Get(leveldb::ReadOptions(), token_key, &stored_owner);
    if (!status.ok()) {
        result.error = "token not minted";
        return result;
    }

    address current_owner{};
    if (!decode_address(stored_owner, current_owner)) {
        result.error = "corrupt owner entry";
        return result;
    }

    std::string token_approval;
    db_->Get(leveldb::ReadOptions(), encode_token_approval_key(token_id), &token_approval);
    address approved_operator{};
    bool has_token_approval = decode_address(token_approval, approved_operator);

    std::string operator_flag;
    const bool operator_whitelisted = db_->Get(leveldb::ReadOptions(), encode_operator_key(current_owner, from), &operator_flag).ok();

    if (!addresses_equal(current_owner, from) && !operator_whitelisted && (!has_token_approval || !addresses_equal(approved_operator, from))) {
        result.error = "transfer not authorized";
        return result;
    }

    leveldb::WriteOptions write_opts;
    status = db_->Put(write_opts, token_key, encode_address(to));
    if (!status.ok()) {
        result.error = status.ToString();
        return result;
    }

    db_->Delete(write_opts, encode_token_approval_key(token_id));

    result.success = true;
    result.owner = to;
    return result;
}

nft_result nft_precompile::approve(const address& owner, const address& operator_addr, const uint256& token_id) {
    nft_result result{};
    result.gas_used = k_gas_approval;

    const std::string token_key = encode_token_key(token_id);
    std::string stored_owner;
    auto status = db_->Get(leveldb::ReadOptions(), token_key, &stored_owner);
    if (!status.ok()) {
        result.error = "token not minted";
        return result;
    }
    address current_owner{};
    if (!decode_address(stored_owner, current_owner) || !addresses_equal(current_owner, owner)) {
        result.error = "approve not authorized";
        return result;
    }

    status = db_->Put(leveldb::WriteOptions(), encode_token_approval_key(token_id), encode_address(operator_addr));
    if (!status.ok()) {
        result.error = status.ToString();
        return result;
    }

    result.success = true;
    result.approved = true;
    return result;
}

nft_result nft_precompile::set_approval_for_all(const address& owner, const address& operator_addr, bool approved) {
    nft_result result{};
    result.gas_used = k_gas_approval;

    const std::string key = encode_operator_key(owner, operator_addr);
    leveldb::WriteOptions write_opts;
    if (approved) {
        auto status = db_->Put(write_opts, key, "1");
        if (!status.ok()) {
            result.error = status.ToString();
            return result;
        }
    } else {
        db_->Delete(write_opts, key);
    }

    result.success = true;
    result.approved = approved;
    return result;
}

nft_result nft_precompile::token_uri(const uint256& token_id) const {
    nft_result result{};
    result.gas_used = k_gas_metadata;

    std::string uri;
    auto status = db_->Get(leveldb::ReadOptions(), encode_metadata_key(token_id), &uri);
    if (!status.ok()) {
        result.error = "metadata not set";
        return result;
    }

    result.success = true;
    result.metadata_uri = uri;
    return result;
}

nft_result nft_precompile::owner_of(const uint256& token_id) const {
    nft_result result{};
    result.gas_used = k_gas_owner_of;

    const std::string token_key = encode_token_key(token_id);
    std::string stored_owner;
    auto status = db_->Get(leveldb::ReadOptions(), token_key, &stored_owner);
    if (!status.ok()) {
        result.error = "token not minted";
        return result;
    }

    address owner_address{};
    if (!decode_address(stored_owner, owner_address)) {
        result.error = "corrupt owner entry";
        return result;
    }

    result.success = true;
    result.owner = owner_address;
    return result;
}

/*
Example Solidity-like wrapper (for illustration):

pragma solidity ^0.8.0;
contract NativeNFT {
    function mint(uint256 tokenId, address to) external {
        // Calls the NFT precompile address 0x00000000000000000000000000000000000000F1
    }
    function transfer(address from, address to, uint256 tokenId) external {
        // Delegates to the precompile transfer
    }
    function ownerOf(uint256 tokenId) external view returns (address) {
        // Reads via the precompile
    }
}

Example JSON-RPC invocation against the precompile (eth_call style):
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "eth_call",
  "params": [{
    "to": "0x00000000000000000000000000000000000000F1",
    "data": "0x6352211e" // ownerOf selector + encoded token id
  }, "latest"]
}
*/

}  // namespace sidechain::precompiles

