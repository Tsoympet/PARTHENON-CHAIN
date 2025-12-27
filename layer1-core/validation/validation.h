#pragma once
#include "../block/block.h"
#include "../consensus/params.h"
#include <functional>
#include <optional>
#include <cstdint>
#include <ctime>

using UTXOLookup = std::function<std::optional<TxOut>(const OutPoint&)>;

struct BlockValidationOptions {
    // Median time past over the last 11 blocks. Zero disables the check
    // (useful for isolated unit tests).
    uint32_t medianTimePast = 0;

    // Current wall clock (or network-adjusted) time. Defaults to now().
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));

    // Maximum allowed drift in seconds into the future.
    uint32_t maxFutureDrift = 2 * 60 * 60; // 2 hours
};

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params, const BlockValidationOptions& opts = {});
bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup = {});
bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup = {}, const BlockValidationOptions& opts = {});
