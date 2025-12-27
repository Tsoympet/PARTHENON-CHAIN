#pragma once

#include "../../layer1-core/tx/transaction.h"
#include <string>
#include <unordered_map>

namespace txindex {

class TxIndex {
public:
    void Add(const uint256& hash, uint32_t height);
    bool Lookup(const uint256& hash, uint32_t& heightOut) const;
    void Save(const std::string& path) const;
    void Load(const std::string& path);
    void AddBlock(const uint256& blockHash, uint32_t height);
    bool LookupBlock(const uint256& blockHash, uint32_t& heightOut) const;

private:
    struct ArrayHasher {
        size_t operator()(const uint256& data) const noexcept;
    };

    std::unordered_map<uint256, uint32_t, ArrayHasher> m_index;
    std::unordered_map<uint256, uint32_t, ArrayHasher> m_blockIndex;
};

} // namespace txindex
