#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <boost/multiprecision/cpp_int.hpp>
#include <leveldb/db.h>

namespace sidechain::precompiles {

using uint256 = boost::multiprecision::cpp_int;
using address = std::array<uint8_t, 20>;

struct nft_result {
    bool success{false};
    uint64_t gas_used{0};
    std::optional<address> owner; // filled for owner_of
    std::optional<std::string> metadata_uri;
    bool approved{false};
    std::string error;
};

class nft_precompile {
public:
    explicit nft_precompile(const std::string& db_path);

    nft_result mint(const uint256& token_id, const address& to, const std::string& metadata_uri);
    nft_result burn(const address& owner, const uint256& token_id);
    nft_result transfer(const address& from, const address& to, const uint256& token_id);
    nft_result approve(const address& owner, const address& operator_addr, const uint256& token_id);
    nft_result set_approval_for_all(const address& owner, const address& operator_addr, bool approved);
    nft_result token_uri(const uint256& token_id) const;
    nft_result owner_of(const uint256& token_id) const;

private:
    std::string encode_token_key(const uint256& token_id) const;
    std::string encode_metadata_key(const uint256& token_id) const;
    std::string encode_operator_key(const address& owner, const address& operator_addr) const;
    std::string encode_token_approval_key(const uint256& token_id) const;
    static std::string encode_address(const address& addr);
    static bool decode_address(const std::string& value, address& out);

    std::unique_ptr<leveldb::DB> db_;
};

}  // namespace sidechain::precompiles

