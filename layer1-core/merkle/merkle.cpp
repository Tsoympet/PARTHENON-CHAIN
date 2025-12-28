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

    while (layer.size() > 1) {
        if (layer.size() % 2 != 0)
            layer.push_back(layer.back());

        std::vector<uint256> next;
        next.reserve(layer.size() / 2);
        for (size_t i = 0; i < layer.size(); i += 2) {
            uint8_t concat[64];
            std::memcpy(concat, layer[i].data(), 32);
            std::memcpy(concat + 32, layer[i + 1].data(), 32);
            next.push_back(tagged_hash("MERKLE", concat, sizeof(concat)));
        }
        layer.swap(next);
    }
    return layer.front();
}
