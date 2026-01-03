#pragma once
#include "../block/block.h"
#include "../consensus/params.h"
#include "anti_dos.h"
#include <array>
#include <functional>
#include <optional>
#include <cstdint>
#include <ctime>

using UTXOLookup = std::function<std::optional<TxOut>(const OutPoint&)>;

struct BlockValidationOptions {
    // Median time past over the last 11 blocks. Must be provided to enforce
    // BIP113-style timestamp ordering.
    uint32_t medianTimePast = 0;

    // Current wall clock (or network-adjusted) time. Defaults to now().
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));

    // Maximum allowed drift in seconds into the future.
    uint32_t maxFutureDrift = 2 * 60 * 60; // 2 hours

    // Optional rate limiter used to gate validation to avoid DoS. If provided
    // and Consume fails, validation short-circuits.
    ValidationRateLimiter* limiter = nullptr;

    // Weight to charge against the limiter per block (defaults to 1 request).
    uint64_t limiterWeight = 1;

    // Optional Layer-2 anchor validation. When enabled, blocks must supply a
    // non-zero nft_state_root and optionally match an expected anchor.
    bool requireNftStateRoot = false;
    std::array<uint8_t, 32> nftStateRoot{};
    std::array<uint8_t, 32> expectedNftStateRoot{};
};

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params, const BlockValidationOptions& opts = {}, bool skipPowCheck = false);
bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup = {}, bool posMode = false, uint32_t posBits = 0, uint32_t posTime = 0);
bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup = {}, const BlockValidationOptions& opts = {});
