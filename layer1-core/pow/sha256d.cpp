#include "sha256d.h"

#include <openssl/sha.h>

#include <cstring>

Hash256 SHA256(const uint8_t* data, size_t len) {
    Hash256 hash{};
    if (!data) {
        return hash;
    }

    SHA256_CTX ctx{};
    if (SHA256_Init(&ctx) != 1 || SHA256_Update(&ctx, data, len) != 1 ||
        SHA256_Final(hash.data(), &ctx) != 1) {
        hash.fill(0);
        return hash;
    }
    return hash;
}

void sha256d(uint8_t hash[32], const uint8_t* data, size_t len) {
    if (!hash) {
        return;
    }

    uint8_t first[32]{};
    SHA256_CTX ctx{};

    if (data) {
        if (SHA256_Init(&ctx) != 1 || SHA256_Update(&ctx, data, len) != 1 ||
            SHA256_Final(first, &ctx) != 1) {
            std::memset(hash, 0, 32);
            return;
        }
    }

    if (SHA256_Init(&ctx) != 1 || SHA256_Update(&ctx, first, sizeof(first)) != 1 ||
        SHA256_Final(hash, &ctx) != 1) {
        std::memset(hash, 0, 32);
    }
}

Hash256 SHA256d(const uint8_t* data, size_t len) {
    Hash256 out{};
    sha256d(out.data(), data, len);
    return out;
}

bool check_pow(const uint8_t hash[32], const uint256& target) {
    if (!hash) {
        return false;
    }

    // Both hash and target are big-endian; compare lexicographically.
    const int cmp = std::memcmp(hash, target.data(), target.size());
    return cmp < 0;
}

