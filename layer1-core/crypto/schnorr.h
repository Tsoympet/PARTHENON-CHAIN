#pragma once
#include <array>
#include <vector>
#include <cstdint>

bool VerifySchnorr(
    const std::array<uint8_t,32>& pubkey,
    const std::array<uint8_t,64>& sig,
    const std::vector<uint8_t>& msg
);
