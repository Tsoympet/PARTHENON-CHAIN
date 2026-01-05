#include "merkle.h"

#include "../crypto/tagged_hash.h"

#include <cstring>
#include <stdexcept>

uint256 ComputeMerkleRoot(const std::vector<Transaction>& txs)
{
    if (txs.empty())
        return uint256{};

    std::vector<uint256> layer;
    layer.reserve(txs.size());
    for (const auto& tx : txs) {
        layer.push_back(TransactionHash(tx));
    }

    // Optimize: use single allocation for concat buffer outside loop
    uint8_t concat[64];
    while (layer.size() > 1) {
        const size_t layerSize = layer.size();
        const size_t nextSize = (layerSize + 1) / 2;
        
        std::vector<uint256> next;
        next.reserve(nextSize);
        
        for (size_t i = 0; i < layerSize; i += 2) {
            std::memcpy(concat, layer[i].data(), 32);
            // Handle odd-sized layer by duplicating last element
            const size_t rightIdx = (i + 1 < layerSize) ? i + 1 : i;
            std::memcpy(concat + 32, layer[rightIdx].data(), 32);
            next.push_back(tagged_hash("MERKLE", concat, sizeof(concat)));
        }
        layer.swap(next);
    }
    return layer.front();
}
