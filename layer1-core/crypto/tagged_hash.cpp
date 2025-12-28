#include "tagged_hash.h"

#include <openssl/sha.h>

#include <array>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace {

// Helper: compute SHA256 over arbitrary buffers.
static bool sha256_once(const uint8_t* data, size_t len, uint8_t out[32]) {
    if (!data || !out) {
        return false;
    }
    SHA256_CTX ctx{};
    return SHA256_Init(&ctx) == 1 &&
           SHA256_Update(&ctx, data, len) == 1 &&
           SHA256_Final(out, &ctx) == 1;
}

}  // namespace

uint256 tagged_hash(const std::string& tag, const uint8_t* data, size_t size) {
    uint256 result{};

    static std::unordered_map<std::string, std::array<uint8_t, 32>> tag_cache;
    static std::mutex cache_mu;

    std::array<uint8_t, 32> tag_digest{};
    {
        std::lock_guard<std::mutex> l(cache_mu);
        auto it = tag_cache.find(tag);
        if (it != tag_cache.end()) {
            tag_digest = it->second;
        } else {
            if (!sha256_once(reinterpret_cast<const uint8_t*>(tag.data()), tag.size(), tag_digest.data())) {
                return result;
            }
            tag_cache.emplace(tag, tag_digest);
        }
    }

    // Prepare SHA256(tag_hash || tag_hash || data).
    SHA256_CTX ctx{};
    if (SHA256_Init(&ctx) != 1) {
        return result;
    }

    const bool updated = SHA256_Update(&ctx, tag_digest, sizeof(tag_digest)) == 1 &&
                         SHA256_Update(&ctx, tag_digest, sizeof(tag_digest)) == 1 &&
                         (size == 0 || SHA256_Update(&ctx, data, size) == 1);

    if (!updated || SHA256_Final(result.data(), &ctx) != 1) {
        result.fill(0);
    }
    return result;
}
