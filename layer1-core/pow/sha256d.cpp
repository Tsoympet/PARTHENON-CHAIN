#include "sha256d.h"
#include <openssl/sha.h>

Hash256 SHA256(const uint8_t* data, size_t len) {
    Hash256 hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash.data(), &ctx);
    return hash;
}

Hash256 SHA256d(const uint8_t* data, size_t len) {
    auto first = SHA256(data, len);
    return SHA256(first.data(), first.size());
}

