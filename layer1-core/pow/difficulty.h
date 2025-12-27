#pragma once
#include <cstdint>
#include <boost/multiprecision/cpp_int.hpp>
#include "sha256d.h"
#include "../consensus/params.h"
#include "../crypto/tagged_hash.h"

namespace powalgo {
uint32_t CalculateNextWorkRequired(uint32_t lastBits, int64_t actualTimespan, const consensus::Params& params);
bool CheckProofOfWork(const uint256& hash, uint32_t nBits, const consensus::Params& params);
boost::multiprecision::cpp_int CalculateBlockWork(uint32_t nBits);
}
