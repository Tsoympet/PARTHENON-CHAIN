#pragma once
#include <vector>
#include <cstdint>
#include "../tx/transaction.h"
#include "../crypto/tagged_hash.h"

struct BlockHeader {
    uint32_t version;
    uint256  prevBlockHash;
    uint256  merkleRoot;
    uint32_t time;
    uint32_t bits;
    uint32_t nonce;
};

struct Block {
    BlockHeader header;
    std::vector<Transaction> transactions;
};

// Compute the tagged hash of a block header (double-tagged SHA-256 per DRACHMA rules).
uint256 BlockHash(const BlockHeader& header);
