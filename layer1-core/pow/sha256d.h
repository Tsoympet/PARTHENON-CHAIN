#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

using Hash256 = std::array<uint8_t, 32>;
using uint256 = Hash256;

// One-shot SHA-256.
Hash256 SHA256(const uint8_t* data, size_t len);

// Double-SHA256 (SHA256d) used for block header hashing.
Hash256 SHA256d(const uint8_t* data, size_t len);
void sha256d(uint8_t hash[32], const uint8_t* data, size_t len);

// Proof-of-work comparison: returns true if hash < target (both big-endian).
bool check_pow(const uint8_t hash[32], const uint256& target);
