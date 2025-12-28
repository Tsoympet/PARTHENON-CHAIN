#include "block.h"

#include "../crypto/tagged_hash.h"

#include <span>

uint256 BlockHash(const BlockHeader& header) {
    return tagged_hash("BLOCK",
        std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&header), sizeof(BlockHeader))
    );
}

