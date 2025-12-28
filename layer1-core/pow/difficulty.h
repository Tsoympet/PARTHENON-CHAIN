#pragma once
#include <cstdint>
#include <boost/multiprecision/cpp_int.hpp>
#include "sha256d.h"
#include "../consensus/params.h"
#include "../crypto/tagged_hash.h"

namespace powalgo {
struct BlockIndex {
    uint32_t time{0};
    uint32_t bits{0};
    int height{0};
    const BlockIndex* prev{nullptr};

    const BlockIndex* GetAncestor(int target_height) const;
};

uint32_t CalculateNextWorkRequired(uint32_t lastBits, int64_t actualTimespan, const consensus::Params& params);
bool CheckProofOfWork(const uint256& hash, uint32_t nBits, const consensus::Params& params);
boost::multiprecision::cpp_int CalculateBlockWork(uint32_t nBits);
uint32_t calculate_next_work_required(const consensus::Params& params, const BlockIndex* prev);
}
