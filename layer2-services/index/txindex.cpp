#include "txindex.h"

#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace txindex {

TxIndex::TxIndex() = default;

static std::string Hex(const uint256& h)
{
    std::ostringstream ss;
    for (auto b : h) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

std::string TxIndex::KeyFor(const uint256& h, char prefix)
{
    std::string key(1, prefix);
    key += Hex(h);
    return key;
}

void TxIndex::Open(const std::string& path)
{
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::DB* raw{nullptr};
    auto status = leveldb::DB::Open(opts, path, &raw);
    if (!status.ok()) throw std::runtime_error(status.ToString());
    m_db.reset(raw);
}

void TxIndex::Add(const uint256& hash, uint32_t height)
{
    if (!m_db) return;
    auto key = KeyFor(hash, 't');
    leveldb::Slice val(reinterpret_cast<const char*>(&height), sizeof(height));
    m_db->Put(leveldb::WriteOptions{}, key, val);
}

bool TxIndex::Lookup(const uint256& hash, uint32_t& heightOut) const
{
    if (!m_db) return false;
    std::string val;
    auto key = KeyFor(hash, 't');
    auto status = m_db->Get(leveldb::ReadOptions{}, key, &val);
    if (!status.ok()) return false;
    if (val.size() != sizeof(uint32_t)) return false;
    std::memcpy(&heightOut, val.data(), sizeof(uint32_t));
    return true;
}

void TxIndex::AddBlock(const uint256& blockHash, uint32_t height)
{
    m_blockCache[blockHash] = height;
    if (!m_db) return;
    auto key = KeyFor(blockHash, 'b');
    leveldb::Slice val(reinterpret_cast<const char*>(&height), sizeof(height));
    m_db->Put(leveldb::WriteOptions{}, key, val);
}

bool TxIndex::LookupBlock(const uint256& blockHash, uint32_t& heightOut) const
{
    auto it = m_blockCache.find(blockHash);
    if (it != m_blockCache.end()) { heightOut = it->second; return true; }
    if (!m_db) return false;
    std::string val;
    auto key = KeyFor(blockHash, 'b');
    auto status = m_db->Get(leveldb::ReadOptions{}, key, &val);
    if (!status.ok()) return false;
    if (val.size() != sizeof(uint32_t)) return false;
    std::memcpy(&heightOut, val.data(), sizeof(uint32_t));
    return true;
}

} // namespace txindex

