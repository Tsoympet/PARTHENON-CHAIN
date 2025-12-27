#pragma once
#include <cstdint>
#include "sha256d.h"
#include "../consensus/params.h"

namespace pow {
uint32_t CalculateNextWorkRequired(uint32_t lastBits, int64_t actualTimespan, const consensus::Params& params);
bool CheckProofOfWork(const uint256& hash, uint32_t nBits, const consensus::Params& params);
}
