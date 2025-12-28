#include "block.h"

#include "../crypto/tagged_hash.h"

uint256 BlockHash(const BlockHeader& header) {
    return tagged_hash("BLOCK", reinterpret_cast<const uint8_t*>(&header), sizeof(BlockHeader));
}

