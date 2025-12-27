#include <vector>
#include <array>
#include <openssl/sha.h>
#include <stdexcept>

namespace crosschain {

struct HeaderProof {
    std::array<uint8_t,80> header; // simplified block header
    uint32_t height;
};

class ProofValidator {
public:
    bool ValidateChain(const std::vector<HeaderProof>& proofs, const std::array<uint8_t,32>& expectedTip)
    {
        if (proofs.empty()) return false;
        // compute chained hashes (double SHA256) to confirm linkage
        std::array<uint8_t,32> prev{};
        for (size_t i = 0; i < proofs.size(); ++i) {
            std::array<uint8_t,32> hash{};
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, proofs[i].header.data(), proofs[i].header.size());
            SHA256_Final(hash.data(), &ctx);
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, hash.data(), hash.size());
            SHA256_Final(hash.data(), &ctx);
            if (i > 0 && hash != prev)
                return false; // discontinuity
            prev = hash;
        }
        return prev == expectedTip;
    }
};

} // namespace crosschain
