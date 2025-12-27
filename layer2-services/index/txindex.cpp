#include <fstream>
#include <unordered_map>
#include <string>
#include <array>
#include <stdexcept>
#include <cstddef>

#include "../../layer1-core/tx/transaction.h"

namespace txindex {

struct ArrayHasher {
    size_t operator()(const std::array<uint8_t,32>& data) const noexcept
    {
        size_t h = 0;
        for (auto b : data)
            h = (h * 131) ^ b;
        return h;
    }
};

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
    std::unordered_map<uint256, uint32_t, ArrayHasher> m_index;
};

} // namespace txindex
