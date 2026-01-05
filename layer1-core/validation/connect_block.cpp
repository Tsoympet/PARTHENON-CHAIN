#include "validation.h"
#include "../chainstate/coins.h"
#include <unordered_set>
#include <stdexcept>

namespace validation {

namespace {

struct OutPointHasher {
    std::size_t operator()(const OutPoint& o) const noexcept {
        size_t h = 0;
        for (auto b : o.hash) h = (h * 131) ^ b;
        h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
        return h;
    }
};

struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept {
        return a.index == b.index && a.hash == b.hash;
    }
};

} // namespace

// ConnectBlock applies a validated block to the UTXO set and checks that
// all inputs are available and signed correctly.
bool ConnectBlock(const Block& block, 
                  chainstate::Chainstate& chainstate,
                  const consensus::Params& params,
                  int height,
                  const UTXOLookup& fallbackLookup)
{
    // First, validate the block structure and PoW
    BlockValidationOptions opts;
    opts.medianTimePast = 1; // Caller must provide proper MTP
    if (!ValidateBlock(block, params, height, fallbackLookup, opts)) {
        return false;
    }

    // Track all inputs spent in this block to detect double-spends within block
    std::unordered_set<OutPoint, OutPointHasher, OutPointEq> spentInBlock;

    // Process all transactions
    for (size_t txIdx = 0; txIdx < block.transactions.size(); ++txIdx) {
        const auto& tx = block.transactions[txIdx];
        
        // Detect coinbase: must be first tx with single input that has null prevout
        bool isCoinbase = false;
        if (txIdx == 0 && tx.vin.size() == 1) {
            const auto& input = tx.vin[0];
            // Check for null hash (all zeros) and max index
            bool hasNullHash = true;
            for (auto b : input.prevout.hash) {
                if (b != 0) {
                    hasNullHash = false;
                    break;
                }
            }
            isCoinbase = hasNullHash && input.prevout.index == std::numeric_limits<uint32_t>::max();
        }
        
        if (!isCoinbase) {
            // Verify all inputs exist and aren't double-spent
            for (const auto& input : tx.vin) {
                // Check for intra-block double spend
                if (spentInBlock.count(input.prevout)) {
                    return false; // Double spend within this block
                }
                spentInBlock.insert(input.prevout);
                
                // Verify UTXO exists (either in chainstate or fallback)
                std::optional<TxOut> utxo = chainstate.TryGetUTXO(input.prevout);
                if (!utxo && fallbackLookup) {
                    utxo = fallbackLookup(input.prevout);
                }
                if (!utxo) {
                    return false; // Missing UTXO
                }
            }
        }
        
        // Add new outputs to UTXO set
        auto txHash = tx.GetHash();
        for (size_t outIdx = 0; outIdx < tx.vout.size(); ++outIdx) {
            OutPoint op{txHash, static_cast<uint32_t>(outIdx)};
            chainstate.AddUTXO(op, tx.vout[outIdx]);
        }
        
        // Remove spent inputs from UTXO set
        if (!isCoinbase) {
            for (const auto& input : tx.vin) {
                chainstate.SpendUTXO(input.prevout);
            }
        }
    }

    return true;
}

} // namespace validation
