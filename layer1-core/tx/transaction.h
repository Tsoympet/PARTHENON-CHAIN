#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <string>
#include "../crypto/tagged_hash.h"

enum class AssetId : uint8_t { TALANTON = 0, DRACHMA = 1, OBOLOS = 2 };

inline bool IsValidAssetId(uint8_t id)
{
    return id <= static_cast<uint8_t>(AssetId::OBOLOS);
}

struct OutPoint {
    uint256 hash;
    uint32_t index;
};

struct TxIn {
    OutPoint prevout;
    std::vector<uint8_t> scriptSig;
    uint32_t sequence{0xffffffff};
    uint8_t assetId{static_cast<uint8_t>(AssetId::DRACHMA)};
};

struct TxOut {
    uint64_t value;
    std::vector<uint8_t> scriptPubKey;
    uint8_t assetId{static_cast<uint8_t>(AssetId::DRACHMA)};
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
std::array<uint8_t, 32> ComputeInputDigest(const Transaction& tx, size_t inputIndex);

// Utility for tagged hash of a transaction
uint256 TransactionHash(const Transaction& tx);
