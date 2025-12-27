#include <fstream>
#include <unordered_map>
#include <string>
#include <array>
#include <stdexcept>

#include "../../layer1-core/tx/transaction.h"

namespace index {

class TxIndex {
public:
    void Add(const uint256& hash, uint32_t height)
    {
        m_index[hash] = height;
    }

    bool Lookup(const uint256& hash, uint32_t& heightOut) const
    {
        auto it = m_index.find(hash);
        if (it == m_index.end()) return false;
        heightOut = it->second;
        return true;
    }

    void Save(const std::string& path) const
    {
        std::ofstream file(path, std::ios::binary);
        for (const auto& kv : m_index) {
            file.write(reinterpret_cast<const char*>(kv.first.data()), kv.first.size());
            uint32_t h = kv.second;
            file.write(reinterpret_cast<const char*>(&h), sizeof(h));
        }
    }

    void Load(const std::string& path)
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

private:
    std::unordered_map<uint256, uint32_t> m_index;
};

} // namespace index
