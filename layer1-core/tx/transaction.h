#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "../crypto/tagged_hash.h"

struct OutPoint {
    uint256 hash;
    uint32_t index;
};

struct TxIn {
    OutPoint prevout;
    std::vector<uint8_t> scriptSig;
    uint32_t sequence{0xffffffff};
};

struct TxOut {
    uint64_t value;
    std::vector<uint8_t> scriptPubKey;
};

struct Transaction {
    uint32_t version{1};
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    uint32_t lockTime{0};

    uint256 GetHash() const;
};

// Serialization helpers
std::vector<uint8_t> Serialize(const Transaction& tx);
Transaction DeserializeTransaction(const std::vector<uint8_t>& data);

// Utility for tagged hash of a transaction
uint256 TransactionHash(const Transaction& tx);
