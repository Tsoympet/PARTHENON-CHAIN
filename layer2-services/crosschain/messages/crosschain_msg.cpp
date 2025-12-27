#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <openssl/sha.h>

namespace crosschain {

struct CrossChainMessage {
    uint32_t version{1};
    std::string source;
    std::string destination;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> Serialize() const
    {
        std::vector<uint8_t> out;
        auto writeStr = [&out](const std::string& s){
            uint32_t len = static_cast<uint32_t>(s.size());
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&len), reinterpret_cast<uint8_t*>(&len) + sizeof(len));
            out.insert(out.end(), s.begin(), s.end());
        };
        out.insert(out.end(), reinterpret_cast<const uint8_t*>(&version), reinterpret_cast<const uint8_t*>(&version) + sizeof(version));
        writeStr(source);
        writeStr(destination);
        uint32_t len = static_cast<uint32_t>(payload.size());
        out.insert(out.end(), reinterpret_cast<uint8_t*>(&len), reinterpret_cast<uint8_t*>(&len) + sizeof(len));
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    std::array<uint8_t,32> Hash() const
    {
        auto data = Serialize();
        std::array<uint8_t,32> out{};
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, data.data(), data.size());
        SHA256_Final(out.data(), &ctx);
        return out;
    }
};

} // namespace crosschain
