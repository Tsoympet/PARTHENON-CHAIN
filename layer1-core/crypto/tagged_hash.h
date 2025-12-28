#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

using uint256 = std::array<uint8_t, 32>;
// BIP-340 tagged hash: SHA256(SHA256(tag) || SHA256(tag) || data)
// The tag hash is computed once per invocation to avoid repeated hashing for
// callers that supply distinct tags. Data is interpreted as a big-endian
// buffer and not modified.
uint256 tagged_hash(const std::string& tag, const uint8_t* data, size_t size);
