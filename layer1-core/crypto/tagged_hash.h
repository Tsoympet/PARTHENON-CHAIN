#pragma once
#include <array>
#include <cstddef>
#include <string>
#include "../pow/sha256d.h"

using uint256 = Hash256;

uint256 TaggedHash(
    const std::string& tag,
    const uint8_t* data,
    size_t len
);
