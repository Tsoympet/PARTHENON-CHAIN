#pragma once

#include "../../layer1-core/tx/transaction.h"
#include <leveldb/db.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace txindex {

class TxIndex {
public:
    TxIndex();
    void Open(const std::string& path);
    void Add(const uint256& hash, uint32_t height);
    bool Lookup(const uint256& hash, uint32_t& heightOut) const;
    void AddBlock(const uint256& blockHash, uint32_t height);
    bool LookupBlock(const uint256& blockHash, uint32_t& heightOut) const;
    size_t BlockCount() const { return m_blockCache.size(); }

private:
    static std::string KeyFor(const uint256& h, char prefix);
    struct ArrayHasher {
        size_t operator()(const uint256& data) const noexcept {
            size_t h = 0; for (auto b : data) h = (h * 131) ^ b; return h;
        }
    };

    std::unique_ptr<leveldb::DB> m_db;
    std::unordered_map<uint256, uint32_t, ArrayHasher> m_blockCache;
};

} // namespace txindex
