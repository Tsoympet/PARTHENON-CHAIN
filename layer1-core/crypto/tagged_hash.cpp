#include "tagged_hash.h"

#include <openssl/sha.h>

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
    uint8_t tag_digest[32]{};
    uint256 result{};

    // Hash the tag string once.
    if (!sha256_once(reinterpret_cast<const uint8_t*>(tag.data()), tag.size(), tag_digest)) {
        return result;
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
