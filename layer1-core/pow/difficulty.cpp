#include "sha256d.h"
#include "../consensus/params.h"
#include "../crypto/tagged_hash.h"
#include <algorithm>
#include <stdexcept>
#include <boost/multiprecision/cpp_int.hpp>

namespace powalgo {
using boost::multiprecision::cpp_int;

static constexpr uint32_t COMPACT_EXPONENT_MASK = 0xFF000000;
static constexpr uint32_t COMPACT_MANTISSA_MASK = 0x007fffff;
static constexpr uint32_t COMPACT_SIGN_MASK = 0x00800000;

const BlockIndex* BlockIndex::GetAncestor(int target_height) const
{
    const BlockIndex* cursor = this;
    while (cursor && cursor->height > target_height) {
        cursor = cursor->prev;
    }
    if (cursor && cursor->height == target_height) {
        return cursor;
    }
    return nullptr;
}

static cpp_int CompactToTarget(uint32_t nBits)
{
    uint32_t exponent = nBits >> 24;
    uint32_t mantissa = nBits & COMPACT_MANTISSA_MASK;
    if (nBits & COMPACT_SIGN_MASK) {
        throw std::runtime_error("Negative compact target");
    }

    cpp_int target = mantissa;
    if (exponent <= 3) {
        target >>= 8 * (3 - exponent);
    } else {
        target <<= 8 * (exponent - 3);
    }
    return target;
}

static uint32_t TargetToCompact(const cpp_int& target)
{
    if (target <= 0) {
        return 0;
    }

    cpp_int tmp = target;
    int size = 0;
    while (tmp > 0) {
        tmp >>= 8;
        ++size;
    }

    cpp_int compact = target;
    if (size > 3) {
        compact >>= 8 * (size - 3);
    }

    if (compact & 0x00800000) {
        compact >>= 8;
        size += 1;
    }

    uint32_t result = static_cast<uint32_t>(compact.convert_to<uint32_t>() & 0x007fffff);
    result |= static_cast<uint32_t>(size) << 24;
    return result;
}

cpp_int CalculateBlockWork(uint32_t nBits)
{
    // Bitcoin-style work calculation: (2^256 - target) / (target + 1) + 1
    cpp_int target = CompactToTarget(nBits);
    cpp_int powLimit("0x10000000000000000000000000000000000000000000000000000000000000000");
    if (target <= 0 || target >= powLimit)
        return 0;

    cpp_int work = (powLimit - target - 1) / (target + 1);
    return work + 1;
}

uint32_t CalculateNextWorkRequired(
    uint32_t lastBits,
    int64_t actualTimespan,
    const consensus::Params& params)
{
    if (params.nPowTargetTimespan == 0)
        throw std::runtime_error("targetTimespan cannot be zero");

    int64_t targetTimespan = params.nPowTargetTimespan;
    // Clamp the adjustment window to avoid extreme difficulty swings that can
    // destabilize block production.
    actualTimespan = std::clamp(
        actualTimespan,
        targetTimespan / 2,
        targetTimespan * 2
    );

    cpp_int lastTarget = CompactToTarget(lastBits);
    cpp_int newTarget = lastTarget * actualTimespan / targetTimespan;

    // powLimit is encoded by genesis bits
    cpp_int powLimit = CompactToTarget(params.nGenesisBits);
    if (newTarget > powLimit)
        newTarget = powLimit;

    return TargetToCompact(newTarget);
}

uint32_t calculate_next_work_required(const consensus::Params& params, const BlockIndex* prev)
{
    if (!prev)
        return params.nGenesisBits;
    if (params.nDifficultyAdjustmentInterval == 0)
        throw std::runtime_error("difficultyAdjustmentInterval cannot be zero");
    const cpp_int powLimit = CompactToTarget(params.nGenesisBits);

    // Emergency minimum difficulty for test networks if the previous block was far in the past.
    if (params.fPowAllowMinDifficultyBlocks && prev->prev) {
        const uint32_t spacing = params.nPowTargetSpacing;
        if (prev->time > prev->prev->time && prev->time - prev->prev->time > spacing * 2) {
            return params.nGenesisBits;
        }
    }

    // Retarget only at the configured interval. Otherwise reuse the prior difficulty.
    const bool on_retarget = ((prev->height + 1) % static_cast<int>(params.nDifficultyAdjustmentInterval) == 0);
    if (!on_retarget) {
        return prev->bits;
    }

    const int window = static_cast<int>(params.nDifficultyAdjustmentInterval) - 1;
    const BlockIndex* first = prev->GetAncestor(prev->height - window);
    if (!first)
        return params.nGenesisBits;

    // Measure the wall-clock span between the first and last block of the interval.
    int64_t actualTimespan = static_cast<int64_t>(prev->time) - static_cast<int64_t>(first->time);
    const int64_t targetTimespan = static_cast<int64_t>(params.nPowTargetTimespan);
    if (targetTimespan <= 0)
        throw std::runtime_error("powTargetTimespan cannot be zero");
    // Bitcoin-style dampening: limit adjustment step to 4x in either direction.
    actualTimespan = std::clamp(actualTimespan, targetTimespan / 4, targetTimespan * 4);

    cpp_int lastTarget = CompactToTarget(prev->bits);
    cpp_int nextTarget = lastTarget * actualTimespan / targetTimespan;
    if (nextTarget > powLimit)
        nextTarget = powLimit;
    return TargetToCompact(nextTarget);
}

bool CheckProofOfWork(const uint256& hash, uint32_t nBits, const consensus::Params& params)
{
    cpp_int target = CompactToTarget(nBits);
    cpp_int powLimit = CompactToTarget(params.nGenesisBits);
    if (target <= 0 || target > powLimit)
        return false;

    cpp_int value = 0;
    for (uint8_t byte : hash) {
        value <<= 8;
        value += byte;
    }
    return value <= target;
}

} // namespace powalgo
