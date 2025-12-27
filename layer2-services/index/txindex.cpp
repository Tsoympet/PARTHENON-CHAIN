#include "txindex.h"

#include <fstream>

namespace txindex {

void TxIndex::Add(const uint256& hash, uint32_t height)
{
    m_index[hash] = height;
}

bool TxIndex::Lookup(const uint256& hash, uint32_t& heightOut) const
{
    auto it = m_index.find(hash);
    if (it == m_index.end()) return false;
    heightOut = it->second;
    return true;
}

void TxIndex::AddBlock(const uint256& blockHash, uint32_t height)
{
    m_blockIndex[blockHash] = height;
}

bool TxIndex::LookupBlock(const uint256& blockHash, uint32_t& heightOut) const
{
    auto it = m_blockIndex.find(blockHash);
    if (it == m_blockIndex.end()) return false;
    heightOut = it->second;
    return true;
}

void TxIndex::Save(const std::string& path) const
{
    std::ofstream file(path, std::ios::binary);
    for (const auto& kv : m_index) {
        file.write(reinterpret_cast<const char*>(kv.first.data()), kv.first.size());
        uint32_t h = kv.second;
        file.write(reinterpret_cast<const char*>(&h), sizeof(h));
    }
}

void TxIndex::Load(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return;
    m_index.clear();
    while (file.peek() != EOF) {
        uint256 hash{};
        uint32_t h{0};
        file.read(reinterpret_cast<char*>(hash.data()), hash.size());
        if (!file) break;
        file.read(reinterpret_cast<char*>(&h), sizeof(h));
        if (!file) break;
        m_index.emplace(hash, h);
    }
}

size_t TxIndex::ArrayHasher::operator()(const uint256& data) const noexcept
{
    size_t h = 0;
    for (auto b : data) h = (h * 131) ^ b;
    return h;
}

} // namespace txindex
