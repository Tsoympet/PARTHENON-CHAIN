#include "../block/block.h"
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <cstring>

class BlockStore {
public:
    explicit BlockStore(const std::string& path): path(path) { LoadIndex(); }

    void WriteBlock(uint32_t height, const Block& block)
    {
        std::lock_guard<std::mutex> l(mu);
        std::ofstream out(path, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("cannot open blockstore");
        auto pos = out.tellp();

        std::vector<uint8_t> buffer;
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&block.header), reinterpret_cast<const uint8_t*>(&block.header) + sizeof(BlockHeader));
        uint32_t txCount = static_cast<uint32_t>(block.transactions.size());
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&txCount), reinterpret_cast<uint8_t*>(&txCount) + sizeof(txCount));
        for (const auto& tx : block.transactions) {
            auto ser = Serialize(tx);
            uint32_t txSize = static_cast<uint32_t>(ser.size());
            buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&txSize), reinterpret_cast<uint8_t*>(&txSize) + sizeof(txSize));
            buffer.insert(buffer.end(), ser.begin(), ser.end());
        }

        uint32_t totalSize = static_cast<uint32_t>(buffer.size());
        out.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        index[height] = static_cast<uint64_t>(pos);
        FlushIndex();
    }

    Block ReadBlock(uint32_t height)
    {
        std::lock_guard<std::mutex> l(mu);
        auto it = index.find(height);
        if (it == index.end()) throw std::runtime_error("unknown height");
        std::ifstream in(path, std::ios::binary);
        in.seekg(it->second);
        uint32_t size = 0;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (!in) throw std::runtime_error("corrupt blockstore");

        Block block{};
        if (size < sizeof(BlockHeader) + sizeof(uint32_t)) throw std::runtime_error("block too small");
        size_t offset = 0;
        std::memcpy(&block.header, data.data() + offset, sizeof(BlockHeader));
        offset += sizeof(BlockHeader);
        uint32_t txCount = 0;
        std::memcpy(&txCount, data.data() + offset, sizeof(txCount));
        offset += sizeof(txCount);
        for (uint32_t i = 0; i < txCount; ++i) {
            if (offset + sizeof(uint32_t) > data.size()) throw std::runtime_error("truncated transaction size");
            uint32_t txSize = 0;
            std::memcpy(&txSize, data.data() + offset, sizeof(txSize));
            offset += sizeof(txSize);
            if (offset + txSize > data.size()) throw std::runtime_error("truncated transaction data");
            std::vector<uint8_t> txdata(data.begin() + offset, data.begin() + offset + txSize);
            offset += txSize;
            block.transactions.push_back(DeserializeTransaction(txdata));
        }
        return block;
    }

private:
    std::string path;
    std::unordered_map<uint32_t, uint64_t> index;
    std::mutex mu;

    void LoadIndex()
    {
        std::ifstream in(path + ".idx", std::ios::binary);
        if (!in.good()) return;
        uint32_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t height; uint64_t off;
            in.read(reinterpret_cast<char*>(&height), sizeof(height));
            in.read(reinterpret_cast<char*>(&off), sizeof(off));
            index[height] = off;
        }
    }

    void FlushIndex()
    {
        std::ofstream out(path + ".idx", std::ios::binary | std::ios::trunc);
        uint32_t count = static_cast<uint32_t>(index.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& e : index) {
            out.write(reinterpret_cast<const char*>(&e.first), sizeof(e.first));
            out.write(reinterpret_cast<const char*>(&e.second), sizeof(e.second));
        }
    }
};
