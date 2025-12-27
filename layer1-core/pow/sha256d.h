#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

using Hash256 = std::array<uint8_t, 32>;

Hash256 SHA256(const uint8_t* data, size_t len);
Hash256 SHA256d(const uint8_t* data, size_t len);
