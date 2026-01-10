#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include "../../layer1-core/crypto/tagged_hash.h"
#include "../../layer1-core/tx/transaction.h"

namespace obolos_test {

// Test doubles for settlement-layer components that are not yet implemented.
// These mirrors use real layer1-core types (Transaction/uint256) so fixtures
// remain compatible with future production types.
struct Checkpoint {
    uint32_t height{0};
    uint256 block_hash{};
    std::chrono::system_clock::time_point created_at{};
};

struct Receipt {
    uint256 tx_id{};
    uint32_t finality_height{0};
    std::string status{"finalized"};
};

struct AccountQuery {
    std::string account_id;
    uint64_t balance{0};
    uint32_t nonce{0};
};

inline uint256 MakeHashFromSeed(uint32_t seed) {
    uint256 hash{};
    for (size_t i = 0; i < hash.size(); ++i) {
        hash[i] = static_cast<uint8_t>((seed + i * 13) & 0xff);
    }
    return hash;
}

inline Transaction MakeFakeTransaction(uint32_t seed) {
    Transaction tx{};
    TxIn input{};
    input.prevout.hash = MakeHashFromSeed(seed);
    input.prevout.index = seed % 4;
    input.scriptSig = {0x51, static_cast<uint8_t>(seed & 0xff)};
    tx.vin.push_back(input);

    TxOut output{};
    output.value = 1000 + seed;
    output.scriptPubKey = {0x53, 0x54, 0x55};
    tx.vout.push_back(output);

    tx.lockTime = seed;
    return tx;
}

inline Checkpoint MakeCheckpoint(uint32_t height, const uint256& hash) {
    return Checkpoint{height, hash, std::chrono::system_clock::now()};
}

inline Receipt MakeReceipt(const Transaction& tx, uint32_t finality_height) {
    return Receipt{tx.GetHash(), finality_height, "finalized"};
}

inline AccountQuery MakeAccountQuery(const std::string& account_id,
                                     uint64_t balance,
                                     uint32_t nonce) {
    return AccountQuery{account_id, balance, nonce};
}

} // namespace obolos_test
