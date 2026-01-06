#include "../block/block.h"
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <openssl/evp.h>

class BlockStore {
public:
    explicit BlockStore(const std::string& path): path(path) { LoadIndex(); }
    
    ~BlockStore() {
        std::lock_guard<std::mutex> l(mu);
        FlushIndex();
    }

    void WriteBlock(uint32_t height, const Block& block)
    {
        std::lock_guard<std::mutex> l(mu);
        std::ofstream out(path, std::ios::binary | std::ios::app);
        if (!out) throw std::runtime_error("cannot open blockstore");
        auto pos = out.tellp();

        std::vector<uint8_t> buffer;
        buffer.reserve(sizeof(BlockHeader) + sizeof(uint32_t) + 4096);
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
        
        // Calculate SHA-256 checksum for integrity verification using modern EVP API
        std::array<uint8_t, 32> checksum;
        unsigned int checksumLen = 0;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx, buffer.data(), buffer.size());
        EVP_DigestFinal_ex(mdctx, checksum.data(), &checksumLen);
        EVP_MD_CTX_free(mdctx);
        
        // Write: [size][checksum][data]
        out.write(reinterpret_cast<const char*>(&totalSize), sizeof(totalSize));
        out.write(reinterpret_cast<const char*>(checksum.data()), checksum.size());
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        
        index[height] = static_cast<uint64_t>(pos);
        ++dirtyCount;
        if (dirtyCount >= kFlushThreshold) {
            FlushIndex();
            dirtyCount = 0;
        }
    }
    
    void Sync() {
        std::lock_guard<std::mutex> l(mu);
        if (dirtyCount > 0) {
            FlushIndex();
            dirtyCount = 0;
        }
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
        
        // Validate block size to prevent allocation attacks
        const uint32_t MAX_BLOCK_SIZE = 100 * 1024 * 1024; // 100MB max
        if (size == 0 || size > MAX_BLOCK_SIZE) {
            throw std::runtime_error("invalid block size");
        }
        
        // Read checksum
        std::array<uint8_t, 32> storedChecksum;
        in.read(reinterpret_cast<char*>(storedChecksum.data()), storedChecksum.size());
        
        // Read block data
        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (!in) throw std::runtime_error("corrupt blockstore");
        
        // Verify checksum for integrity using modern EVP API
        std::array<uint8_t, 32> computedChecksum;
        unsigned int checksumLen = 0;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx, data.data(), data.size());
        EVP_DigestFinal_ex(mdctx, computedChecksum.data(), &checksumLen);
        EVP_MD_CTX_free(mdctx);
        
        if (storedChecksum != computedChecksum) {
            throw std::runtime_error("block checksum mismatch - data corruption detected");
        }

        Block block{};
        if (size < sizeof(BlockHeader) + sizeof(uint32_t)) throw std::runtime_error("block too small");
        size_t offset = 0;
        std::memcpy(&block.header, data.data() + offset, sizeof(BlockHeader));
        offset += sizeof(BlockHeader);
        uint32_t txCount = 0;
        std::memcpy(&txCount, data.data() + offset, sizeof(txCount));
        offset += sizeof(txCount);
        
        // Validate transaction count to prevent memory exhaustion
        const uint32_t MAX_TX_COUNT = 100000;
        if (txCount > MAX_TX_COUNT) {
            throw std::runtime_error("transaction count exceeds maximum");
        }
        
        for (uint32_t i = 0; i < txCount; ++i) {
            if (offset + sizeof(uint32_t) > data.size()) throw std::runtime_error("truncated transaction size");
            uint32_t txSize = 0;
            std::memcpy(&txSize, data.data() + offset, sizeof(txSize));
            offset += sizeof(txSize);
            
            // Validate transaction size
            const uint32_t MAX_TX_SIZE = 10 * 1024 * 1024; // 10MB max per tx
            if (txSize == 0 || txSize > MAX_TX_SIZE) {
                throw std::runtime_error("invalid transaction size");
            }
            
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
    size_t dirtyCount{0};
    static constexpr size_t kFlushThreshold = 100;

    void LoadIndex()
    {
        std::ifstream in(path + ".idx", std::ios::binary);
        if (!in.good()) return;
        uint32_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        // Validate index count to prevent memory exhaustion
        const uint32_t MAX_INDEX_ENTRIES = 10000000; // 10 million blocks max
        if (count > MAX_INDEX_ENTRIES) {
            throw std::runtime_error("index count exceeds maximum");
        }
        
        for (uint32_t i = 0; i < count; ++i) {
            uint32_t height; uint64_t off;
            in.read(reinterpret_cast<char*>(&height), sizeof(height));
            in.read(reinterpret_cast<char*>(&off), sizeof(off));
            if (!in.good()) {
                throw std::runtime_error("corrupt index file");
            }
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
