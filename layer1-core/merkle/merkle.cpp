#include "merkle.h"

uint256 ComputeMerkleRoot(const std::vector<Transaction>& txs) {
    if (txs.empty()) return uint256{};

    std::vector<uint256> layer;
    for (const auto& tx : txs)
        layer.push_back(TaggedHash("TX", (const uint8_t*)&tx, sizeof(tx)));

    while (layer.size() > 1) {
        if (layer.size() % 2 != 0)
            layer.push_back(layer.back());

        std::vector<uint256> next;
        for (size_t i = 0; i < layer.size(); i += 2) {
            next.push_back(
                TaggedHash("MERKLE",
                    layer[i].data(),
                    layer[i].size() + layer[i+1].size()
                )
            );
        }
        layer = next;
    }
    return layer[0];
}

